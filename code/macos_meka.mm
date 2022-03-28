#include <Cocoa/Cocoa.h> // APPKIT
#include <CoreGraphics/CoreGraphics.h> 
#include <mach/mach_time.h> // mach_absolute_time
#include <stdio.h> // printf for debugging purpose
#include <sys/stat.h>
#include <libkern/OSAtomic.h>
#include <pthread.h>
#include <semaphore.h>
#include <Carbon/Carbon.h>
#include <dlfcn.h> // dlsym
#include <metalkit/metalkit.h>
#include <metal/metal.h>

// TODO(joon) introspection?

// NOTE(joon) we need some metal layer even if we use vulkan
#include "meka_metal_shader_shared.h"

#undef internal
#undef assert

#include "meka_types.h"
#include "meka_platform.h"

#include "meka_metal.cpp"
#include "meka.cpp"

// TODO(joon): Get rid of global variables?
global v2 last_mouse_p;
global v2 mouse_diff;

global u64 last_time;

global b32 is_game_running;
global dispatch_semaphore_t semaphore;

internal u64 
mach_time_diff_in_nano_seconds(u64 begin, u64 end, f32 nano_seconds_per_tick)
{
    return (u64)(((end - begin)*nano_seconds_per_tick));
}

PLATFORM_GET_FILE_SIZE(macos_get_file_size) 
{
    u64 result = 0;

    int File = open(filename, O_RDONLY);
    struct stat FileStat;
    fstat(File , &FileStat); 
    result = FileStat.st_size;
    close(File);

    return result;
}

PLATFORM_READ_FILE(debug_macos_read_file)
{
    PlatformReadFileResult Result = {};

    int File = open(filename, O_RDONLY);
    int Error = errno;
    if(File >= 0) // NOTE : If the open() succeded, the return value is non-negative value.
    {
        struct stat FileStat;
        fstat(File , &FileStat); 
        off_t fileSize = FileStat.st_size;

        if(fileSize > 0)
        {
            // TODO/Joon : NO MORE OS LEVEL ALLOCATION!
            Result.size = fileSize;
            Result.memory = (u8 *)malloc(Result.size);
            if(read(File, Result.memory, fileSize) == -1)
            {
                free(Result.memory);
                Result.size = 0;
            }
        }

        close(File);
    }

    return Result;
}

PLATFORM_WRITE_ENTIRE_FILE(debug_macos_write_entire_file)
{
    int file = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);

    if(file >= 0) 
    {
        if(write(file, memory_to_write, size) == -1)
        {
            // TODO(joon) : LOG here
        }

        close(file);
    }
    else
    {
        // TODO(joon) :LOG
        printf("Failed to create file\n");
    }
}

PLATFORM_FREE_FILE_MEMORY(debug_macos_free_file_memory)
{
    free(memory);
}

@interface 
app_delegate : NSObject<NSApplicationDelegate>
@end
@implementation app_delegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    [NSApp stop:nil];

    // Post empty event: without it we can't put application to front
    // for some reason (I get this technique from GLFW source).
    NSAutoreleasePool* pool = [NSAutoreleasePool new];
    NSEvent* event =
        [NSEvent otherEventWithType: NSApplicationDefined
                 location: NSMakePoint(0, 0)
                 modifierFlags: 0
                 timestamp: 0
                 windowNumber: 0
                 context: nil
                 subtype: 0
                 data1: 0
                 data2: 0];
    [NSApp postEvent: event atStart: YES];
    [pool drain];
}

@end

#define check_ns_error(error)\
{\
    if(error)\
    {\
        printf("check_metal_error failed inside the domain: %s code: %ld\n", [error.domain UTF8String], (long)error.code);\
        assert(0);\
    }\
}\


internal CVReturn 
display_link_callback(CVDisplayLinkRef displayLink, const CVTimeStamp* current_time, const CVTimeStamp* output_time,
                CVOptionFlags ignored_0, CVOptionFlags* ignored_1, void* displayLinkContext)
{
    // NOTE(joon) : display link automatically adjust the framerate.
    // TODO(joon) : Find out in what condition it adjusts the framerate?
    u32 last_frame_diff = (u32)(output_time->hostTime - last_time);
    u32 current_time_diff = (u32)(output_time->hostTime - current_time->hostTime);

    f32 last_frame_time_elapsed =last_frame_diff/(f32)output_time->videoTimeScale;
    f32 current_time_elapsed = current_time_diff/(f32)output_time->videoTimeScale;
    
    //printf("last frame diff: %.6f, current time diff: %.6f\n",  last_frame_time_elapsed, current_time_elapsed);

    last_time = output_time->hostTime;
    return kCVReturnSuccess;
}

internal void
macos_handle_event(NSApplication *app, NSWindow *window, PlatformInput *platform_input)
{
    NSPoint mouse_location = [NSEvent mouseLocation];
    NSRect frame_rect = [window frame];
    NSRect content_rect = [window contentLayoutRect];

    v2 bottom_left_p = {};
    bottom_left_p.x = frame_rect.origin.x;
    bottom_left_p.y = frame_rect.origin.y;

    v2 content_rect_dim = {}; 
    content_rect_dim.x = content_rect.size.width; 
    content_rect_dim.y = content_rect.size.height;

    v2 rel_mouse_location = {};
    rel_mouse_location.x = mouse_location.x - bottom_left_p.x;
    rel_mouse_location.y = mouse_location.y - bottom_left_p.y;

    f32 mouse_speed_when_clipped = 0.08f;
    if(rel_mouse_location.x >= 0.0f && rel_mouse_location.x < content_rect_dim.x)
    {
        mouse_diff.x = mouse_location.x - last_mouse_p.x;
    }
    else if(rel_mouse_location.x < 0.0f)
    {
        mouse_diff.x = -mouse_speed_when_clipped;
    }
    else
    {
        mouse_diff.x = mouse_speed_when_clipped;
    }

    if(rel_mouse_location.y >= 0.0f && rel_mouse_location.y < content_rect_dim.y)
    {
        mouse_diff.y = mouse_location.y - last_mouse_p.y;
    }
    else if(rel_mouse_location.y < 0.0f)
    {
        mouse_diff.y = -mouse_speed_when_clipped;
    }
    else
    {
        mouse_diff.y = mouse_speed_when_clipped;
    }

    // NOTE(joon) : MacOS screen coordinate is bottom-up, so just for the convenience, make y to be bottom-up
    mouse_diff.y *= -1.0f;

    last_mouse_p.x = mouse_location.x;
    last_mouse_p.y = mouse_location.y;

    //printf("%f, %f\n", mouse_diff.x, mouse_diff.y);

    // TODO : Check if this loop has memory leak.
    while(1)
    {
        NSEvent *event = [app nextEventMatchingMask:NSAnyEventMask
                         untilDate:nil
                            inMode:NSDefaultRunLoopMode
                           dequeue:YES];
        if(event)
        {
            switch([event type])
            {
                case NSEventTypeKeyUp:
                case NSEventTypeKeyDown:
                {
                    b32 was_down = event.ARepeat;
                    b32 is_down = ([event type] == NSEventTypeKeyDown);

                    if((is_down != was_down) || !is_down)
                    {
                        //printf("isDown : %d, WasDown : %d", is_down, was_down);
                        u16 key_code = [event keyCode];
                        if(key_code == kVK_Escape)
                        {
                            is_game_running = false;
                        }
                        else if(key_code == kVK_ANSI_W)
                        {
                            platform_input->move_up = is_down;
                        }
                        else if(key_code == kVK_ANSI_A)
                        {
                            platform_input->move_left = is_down;
                        }
                        else if(key_code == kVK_ANSI_S)
                        {
                            platform_input->move_down = is_down;
                        }
                        else if(key_code == kVK_ANSI_D)
                        {
                            platform_input->move_right = is_down;
                        }

                        else if(key_code == kVK_ANSI_I)
                        {
                            platform_input->action_up = is_down;
                        }
                        else if(key_code == kVK_ANSI_J)
                        {
                            platform_input->action_left = is_down;
                        }
                        else if(key_code == kVK_ANSI_K)
                        {
                            platform_input->action_down = is_down;
                        }
                        else if(key_code == kVK_ANSI_L)
                        {
                            platform_input->action_right = is_down;
                        }

                        else if(key_code == kVK_LeftArrow)
                        {
                            platform_input->action_left = is_down;
                        }
                        else if(key_code == kVK_RightArrow)
                        {
                            platform_input->action_right = is_down;
                        }
                        else if(key_code == kVK_UpArrow)
                        {
                            platform_input->action_up = is_down;
                        }
                        else if(key_code == kVK_DownArrow)
                        {
                            platform_input->action_down = is_down;
                        }

                        else if(key_code == kVK_Space)
                        {
                            platform_input->space = is_down;
                        }

                        else if(key_code == kVK_Return)
                        {
                            if(is_down)
                            {
                                NSWindow *window = [event window];
                                // TODO : proper buffer resize here!
                                [window toggleFullScreen:0];
                            }
                        }
                    }
                }break;

                default:
                {
                    [app sendEvent : event];
                }
            }
        }
        else
        {
            break;
        }
    }
} 

// TODO(joon) : It seems like this combines read & write barrier, but make sure
// TODO(joon) : mfence?(DSB)
#define write_barrier() OSMemoryBarrier(); 
#define read_barrier() OSMemoryBarrier();

struct macos_thread
{
    u32 ID;
    thread_work_queue *queue;

    // TODO(joon): I like the idea of each thread having a random number generator that they can use throughout the whole process
    // though what should happen to the 0th thread(which does not have this structure)?
    simd_random_series series;
};

// NOTE(joon) : use this to add what thread should do
internal 
THREAD_WORK_CALLBACK(print_string)
{
    char *stringToPrint = (char *)data;
    printf("%s\n", stringToPrint);
}

#if 0
struct thread_work_raytrace_tile_data
{
    raytracer_data raytracer_input;
};

global volatile u64 total_bounced_ray_count;
internal 
THREAD_WORK_CALLBACK(thread_work_callback_render_tile)
{
    thread_work_raytrace_tile_data *raytracer_data = (thread_work_raytrace_tile_data *)data;

    //raytracer_output output = render_raytraced_image_tile(&raytracer_data->raytracer_input);
    raytracer_output output = render_raytraced_image_tile_simd(&raytracer_data->raytracer_input);

    // TODO(joon): double check the return value of the OSAtomicIncrement32, is it really a post incremented value? 
    i32 rendered_tile_count = OSAtomicIncrement32Barrier((volatile int32_t *)&raytracer_data->raytracer_input.world->rendered_tile_count);

    u64 ray_count = raytracer_data->raytracer_input.ray_per_pixel_count*
                    (raytracer_data->raytracer_input.one_past_max_x - raytracer_data->raytracer_input.min_x) * 
                    (raytracer_data->raytracer_input.one_past_max_y - raytracer_data->raytracer_input.min_y);

    OSAtomicAdd64Barrier(ray_count, (volatile int64_t *)&raytracer_data->raytracer_input.world->total_ray_count);
    OSAtomicAdd64Barrier(output.bounced_ray_count, (volatile int64_t *)&raytracer_data->raytracer_input.world->bounced_ray_count);

    printf("%dth tile finished with %llu rays\n", rendered_tile_count, raytracer_data->raytracer_input.world->total_ray_count);
}
#endif

// NOTE(joon): This is single producer multiple consumer - 
// meaning, it _does not_ provide any thread safety
// For example, if the two threads try to add the work item,
// one item might end up over-writing the other one
internal void
macos_add_thread_work_item(thread_work_queue *queue,
                            thread_work_callback *work_callback,
                            void *data)
{
    assert(data); // TODO(joon) : There might be a work that does not need any data?
    thread_work_item *item = queue->items + queue->add_index;
    item->callback = work_callback;
    item->data = data;
    item->written = true;

    write_barrier();
    queue->add_index++;

    // increment the semaphore value by 1
    dispatch_semaphore_signal(semaphore);
}

internal b32
macos_do_thread_work_item(thread_work_queue *queue, u32 thread_index)
{
    b32 did_work = false;
    if(queue->work_index != queue->add_index)
    {
        int original_work_index = queue->work_index;
        int expected_work_index = original_work_index + 1;

        if(OSAtomicCompareAndSwapIntBarrier(original_work_index, expected_work_index, &queue->work_index))
        {
            thread_work_item *item = queue->items + original_work_index;
            item->callback(item->data);

            //printf("Thread %u: Finished working\n", thread_index);
            did_work = true;
        }
    }

    return did_work;
}

internal 
PLATFORM_COMPLETE_ALL_THREAD_WORK_QUEUE_ITEMS(macos_complete_all_thread_work_queue_items)
{
    // TODO(joon): If there was a last thread that was working on the item,
    // this does not guarantee that the last work will be finished.
    // Maybe add some flag inside the thread? (sleep / working / ...)
    while(queue->work_index != queue->add_index) 
    {
        macos_do_thread_work_item(queue, 0);
    }
}


internal void*
thread_proc(void *data)
{
    macos_thread *thread = (macos_thread *)data;
    while(1)
    {
        if(macos_do_thread_work_item(thread->queue, thread->ID))
        {
        }
        else
        {
            // dispatch semaphore puts the thread into sleep until the semaphore is signaled
            dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
        }
    }

    return 0;
}

#if MEKA_VULKAN
internal void
macos_initialize_vulkan(RenderContext *render_context, CAMetalLayer *metal_layer)
{
    void *lib = dlopen("../external/vulkan/macos/lib/libvulkan.1.dylib", RTLD_LAZY|RTLD_GLOBAL);
    if(lib)
    {
        vkGetInstanceProcAddr = (VFType(vkGetInstanceProcAddr))dlsym(lib, "vkGetInstanceProcAddr");
    }
    else
    {
        invalid_code_path;
    }

    resolve_pre_instance_functions();

    char *desired_layers[] = 
    {
#if MEKA_DEBUG
        "VK_LAYER_KHRONOS_validation" 
#endif
    };
    vk_check_desired_layers_support(desired_layers, array_count(desired_layers));

    char *desired_extensions[] =
    {
#if MEKA_DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_METAL_SURFACE_EXTENSION_NAME,
        "VK_KHR_get_physical_device_properties2",
        //"VK_KHR_dynamic_rendering", // TODO(joon) sometimes in the future....
    };
    vk_check_desired_extensions_support(desired_extensions, array_count(desired_extensions));

    VkInstance instance = vk_create_instance(desired_layers, array_count(desired_layers), desired_extensions, array_count(desired_extensions));

    resolve_instance_functions(instance);

    // NOTE(joon) create debug messenger
    CreateDebugMessenger(instance);

    VkPhysicalDevice physical_device = find_physical_device(instance);
    VkPhysicalDeviceProperties property;
    vkGetPhysicalDeviceProperties(physical_device, &property);
    u32 uniform_buffer_alignment = property.limits.minUniformBufferOffsetAlignment;

    VkMetalSurfaceCreateInfoEXT surface_create_info = {};
    surface_create_info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    surface_create_info.pNext = 0;
    surface_create_info.flags = 0;
    surface_create_info.pLayer = metal_layer;
    VkSurfaceKHR surface;
    CheckVkResult(vkCreateMetalSurfaceEXT(instance, &surface_create_info, 0, &surface));

    vk_queue_families queue_families = get_supported_queue_familes(physical_device, surface);
    VkDevice device = vk_create_device(physical_device, surface, &queue_families);

    ResolveDeviceLevelFunctions(device);

    VkQueue graphics_queue;
    VkQueue transfer_queue;
    VkQueue present_queue;
    vkGetDeviceQueue(device, queue_families.graphicsQueueFamilyIndex, 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_families.presentQueueFamilyIndex, 0, &present_queue);
    vkGetDeviceQueue(device, queue_families.transferQueueFamilyIndex, 0, &transfer_queue);

    VkFormat surface_format_candidates[] = {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB};

    VkSurfaceFormatKHR surface_format = vk_select_surface_format(physical_device, surface, surface_format_candidates, array_count(surface_format_candidates));

    vk_check_desired_present_mode_support(physical_device, surface, VK_PRESENT_MODE_FIFO_KHR);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    CheckVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities));

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    //swapchain_create_info.flags; // TODO(joon) : might be worth looking into protected image?
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = surface_capabilities.minImageCount + 1;
    if(swapchain_create_info.minImageCount > surface_capabilities.maxImageCount)
    {
        swapchain_create_info.minImageCount = surface_capabilities.maxImageCount;
    }
    if(surface_capabilities.maxImageCount == 0)
    {
        // NOTE(joon) ; If the maxImageCount is 0, means there is no limit for the maximum image count
        swapchain_create_info.minImageCount = 3;
    }
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_create_info.imageExtent = surface_capabilities.currentExtent;
    swapchain_create_info.imageArrayLayers = 1; // non-stereoscopic
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 1;
    swapchain_create_info.pQueueFamilyIndices = &queue_families.graphicsQueueFamilyIndex; // TODO(joon) why are we just using the graphics queue?
    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    // TODO(joon) : surface_capabilities also return supported alpha composite, so we should check that one too
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; 
    swapchain_create_info.clipped = VK_TRUE;

    VkSwapchainKHR swapchain;
    vkCreateSwapchainKHR(device, &swapchain_create_info, 0, &swapchain);

    // NOTE(joon) : You _have to_ query first and then get the images, otherwise vulkan always returns VK_INCOMPLETE
    u32 swapchain_image_count;
    CheckVkResult(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, 0));
    VkImage *swapchain_images = (VkImage *)malloc(swapchain_image_count*sizeof(VkImage)); // TODO(joon) maybe get rid of this dynamic allocation
    CheckVkResult(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images));

    VkImageView *swapchain_image_views = (VkImageView *)malloc(swapchain_image_count*sizeof(VkImageView));
    for(u32 swapchain_image_index = 0;
        swapchain_image_index < swapchain_image_count;
        ++swapchain_image_index)
    {
        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = swapchain_images[swapchain_image_index];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = surface_format.format;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;

        CheckVkResult(vkCreateImageView(device, &image_view_create_info, 0, swapchain_image_views + swapchain_image_index));
    }

    VkAttachmentReference color_attachemnt_ref = {};
    color_attachemnt_ref.attachment = 0; // index
    color_attachemnt_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // TODO(joon) : Check this value!

    VkAttachmentReference depthStencil_ref = {};
    depthStencil_ref.attachment = 1; // index
    depthStencil_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc = {};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_attachemnt_ref;
    subpass_desc.pDepthStencilAttachment = &depthStencil_ref;

    // NOTE(joon) color attachment
    VkAttachmentDescription renderpass_attachments[2] = {};
    renderpass_attachments[0].format = surface_format.format;
    renderpass_attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    renderpass_attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // This will enable the renderpass clear value
    renderpass_attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE; // If we are not recording the command buffer every single time, OP_STORE should be specified
    renderpass_attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderpass_attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    renderpass_attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    renderpass_attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // NOTE(joon) depth attachment
    renderpass_attachments[1].format = VK_FORMAT_D32_SFLOAT;
    renderpass_attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    renderpass_attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // This will enable the renderpass clear value
    renderpass_attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // If we are not recording the command buffer every single time, OP_STORE should be specified
    renderpass_attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderpass_attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    renderpass_attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    renderpass_attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // NOTE(joon): Not an official information, but pipeline is always optimized
    // for using specific subpass with specific renderpass, not for using 
    // multiple subpasses
    VkRenderPassCreateInfo renderpass_create_info = {};
    renderpass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_create_info.attachmentCount = array_count(renderpass_attachments);
    renderpass_create_info.pAttachments = renderpass_attachments;
    renderpass_create_info.subpassCount = 1;
    renderpass_create_info.pSubpasses = &subpass_desc;
    //renderpass_create_info.dependencyCount = 0;
    //renderpass_create_info.pDependencies = ;
    
    VkRenderPass renderpass;
    CheckVkResult(vkCreateRenderPass(device, &renderpass_create_info, 0, &renderpass));

    PlatformReadFileResult vertex_shader_code = debug_macos_read_file("/Volumes/meka/meka_renderer/code/shader/shader.vert.spv");
    VkShaderModuleCreateInfo vertex_shader_module_create_info = {};
    vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_create_info.codeSize = vertex_shader_code.size;
    vertex_shader_module_create_info.pCode = (u32 *)vertex_shader_code.memory;

    VkShaderModule vertex_shader_module;
    CheckVkResult(vkCreateShaderModule(device, &vertex_shader_module_create_info, 0, &vertex_shader_module));
    //platformApi->FreeFileMemory(vertex_shader_code.memory);

    PlatformReadFileResult frag_shader_code = debug_macos_read_file("/Volumes/meka/meka_renderer/code/shader/shader.frag.spv");
    VkShaderModuleCreateInfo frag_shader_module_create_info = {};
    frag_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_shader_module_create_info.codeSize = frag_shader_code.size;
    frag_shader_module_create_info.pCode = (u32 *)frag_shader_code.memory;

    VkShaderModule frag_shader_module;
    CheckVkResult(vkCreateShaderModule(device, &frag_shader_module_create_info, 0, &frag_shader_module));
    //platformApi->FreeFileMemory(frag_shader_code.memory);

    VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
    stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stage_create_info[0].module = vertex_shader_module;
    stage_create_info[0].pName = "main";

    stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stage_create_info[1].module = frag_shader_module;
    stage_create_info[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {};
    viewport.width = (f32)surface_capabilities.currentExtent.width;
    viewport.height = (f32)surface_capabilities.currentExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.extent.width = surface_capabilities.currentExtent.width;
    scissor.extent.height = surface_capabilities.currentExtent.height;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable = false;
    rasterizationState.rasterizerDiscardEnable = false; // TODO(joon) :If enabled, the rasterizer will discard some vertices, but don't know what's the policy for that
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT; // which triangle should be discarded
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable = false;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT ;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = true;
    depthStencilState.depthWriteEnable = true;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.depthBoundsTestEnable = false;
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = false;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask =   VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = false;
    colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;
    
#if 1
    VkDescriptorSetLayoutBinding layout_bindings[2] = {};
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[1].descriptorCount = 1;
    layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR; // descriptor sets must not be allocated with this layout
    layoutCreateInfo.bindingCount = array_count(layout_bindings);
    layoutCreateInfo.pBindings = layout_bindings;

    VkDescriptorSetLayout setLayout;
    vkCreateDescriptorSetLayout(device, &layoutCreateInfo, 0, &setLayout);
#endif
    VkPushConstantRange pushConstantRange[1] = {};
    pushConstantRange[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange[0].offset = 0;
    pushConstantRange[0].size = sizeof(m4);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &setLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = array_count(pushConstantRange);  // TODO(joon) : Look into this more to use push constant
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRange;

    VkPipelineLayout pipeline_layout;
    CheckVkResult(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, 0, &pipeline_layout));

    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }; 
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO; 
    dynamic_state.dynamicStateCount = array_count(dynamic_states);
    dynamic_state.pDynamicStates = dynamic_states;

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.stageCount = 2;
    graphics_pipeline_create_info.pStages = stage_create_info;
    graphics_pipeline_create_info.pVertexInputState = &vertexInputState; // vertices are pushed using pushDescriptorSet
    graphics_pipeline_create_info.pInputAssemblyState = &inputAssemblyState;
    graphics_pipeline_create_info.pViewportState = &viewportState;
    graphics_pipeline_create_info.pRasterizationState = &rasterizationState;
    graphics_pipeline_create_info.pMultisampleState = &multisampleState;
    graphics_pipeline_create_info.pDepthStencilState = &depthStencilState;
    graphics_pipeline_create_info.pColorBlendState = &colorBlendState;
    graphics_pipeline_create_info.pDynamicState = &dynamic_state;
    graphics_pipeline_create_info.layout = pipeline_layout;
    graphics_pipeline_create_info.renderPass = renderpass;
    graphics_pipeline_create_info.subpass = 0;

    VkPipeline graphics_pipeline;
    CheckVkResult(vkCreateGraphicsPipelines(device, 0, 1, &graphics_pipeline_create_info, 0, &graphics_pipeline));

    // NOTE(joon) now it's safe to destroy the shader modules
    vkDestroyShaderModule(device, vertex_shader_module, 0);
    vkDestroyShaderModule(device, frag_shader_module, 0);

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_D32_SFLOAT;
    image_create_info.extent.width = surface_capabilities.currentExtent.width;
    image_create_info.extent.height = surface_capabilities.currentExtent.height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage depth_image;
    CheckVkResult(vkCreateImage(device, &image_create_info, 0, &depth_image));

    // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, depth_image, &memory_requirements);

    u32 usable_memory_index_for_depth_image = vk_get_usable_memory_index(physical_device, device, memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = usable_memory_index_for_depth_image;

    VkDeviceMemory depth_image_memory;
    CheckVkResult(vkAllocateMemory(device, &memory_allocate_info, 0, &depth_image_memory));

    CheckVkResult(vkBindImageMemory(device, depth_image, depth_image_memory, 0));

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = depth_image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = VK_FORMAT_D32_SFLOAT;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.layerCount = 1;

    VkImageView depth_image_view;
    CheckVkResult(vkCreateImageView(device, &image_view_create_info, 0, &depth_image_view));

    VkFramebuffer *framebuffers = (VkFramebuffer *)malloc(swapchain_image_count * sizeof(VkFramebuffer)); 
    for(u32 framebuffer_index = 0;
        framebuffer_index < swapchain_image_count;
        ++framebuffer_index)
    {
        VkImageView attachments[2] = {};
        attachments[0] = swapchain_image_views[framebuffer_index];
        attachments[1] = depth_image_view;

        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = renderpass;
        framebuffer_create_info.attachmentCount = array_count(attachments);
        framebuffer_create_info.pAttachments = attachments;
        framebuffer_create_info.width = surface_capabilities.currentExtent.width;
        framebuffer_create_info.height = surface_capabilities.currentExtent.height;
        framebuffer_create_info.layers = 1;

        CheckVkResult(vkCreateFramebuffer(device, &framebuffer_create_info, 0, framebuffers + framebuffer_index));
    }

    // NOTE(joon) : command buffers inside the pool can only be submitted to the specified queue
    // graphics related command buffers- 
    // graphics related command buffers -> graphics queue
    // transfer related command buffers -> transfer queue
    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = queue_families.graphicsQueueFamilyIndex;

    VkCommandPool graphics_command_pool;
    CheckVkResult(vkCreateCommandPool(device, &command_pool_create_info, 0, &graphics_command_pool));

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = graphics_command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = swapchain_image_count;

    VkCommandBuffer *graphics_command_buffers = (VkCommandBuffer *)malloc(sizeof(VkCommandBuffer) * swapchain_image_count);
    CheckVkResult(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, graphics_command_buffers));

#if 0
    // TODO(joon) : If possible, make this to use transfer queue(and command pool)!
    VkCommandBuffer transfer_commadn_pool;
    commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = graphicsCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
    CheckVkResult(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &transferCommandBuffer));
#endif
    
    VkSemaphore *ready_to_render_semaphores = (VkSemaphore *)malloc(sizeof(VkSemaphore) * swapchain_image_count);
    VkSemaphore *ready_to_present_semaphores = (VkSemaphore *)malloc(sizeof(VkSemaphore) * swapchain_image_count);
    for(u32 semaphore_index = 0;
        semaphore_index < swapchain_image_count;
        ++semaphore_index)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        CheckVkResult(vkCreateSemaphore(device, &semaphore_create_info, 0, ready_to_render_semaphores + semaphore_index));
        CheckVkResult(vkCreateSemaphore(device, &semaphore_create_info, 0, ready_to_present_semaphores + semaphore_index));
    }

    VkFence *ready_to_render_fences = (VkFence *)malloc(sizeof(VkFence) * swapchain_image_count);
    VkFence *ready_to_present_fences = (VkFence *)malloc(sizeof(VkFence) * swapchain_image_count);
    for(u32 fence_index = 0;
        fence_index < swapchain_image_count;
        ++fence_index)
    {
        VkFenceCreateInfo  fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        
        CheckVkResult(vkCreateFence(device, &fenceCreateInfo, 0, ready_to_render_fences + fence_index));
        CheckVkResult(vkCreateFence(device, &fenceCreateInfo, 0, ready_to_present_fences + fence_index));
    }

    // TODO(joon) : Correct this image memory barrier, and also find out what pipeline stage we should be using
#if 0
    for(u32 swapchainImageIndex = 0;
        swapchainImageIndex < swapchainImageCount;
        ++swapchainImageIndex)
    {
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE_KHR;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // No queue transfer occurs
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.image = swapchainImages[swapchainImageIndex];
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.levelCount = 1; // vk_remaining_mip_levels?
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount = 1; // vk_remaining_array_levels?
        
        vkCmdPipelineBarrier(transferCommandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    1,
                                    &imageMemoryBarrier);
#endif
    render_context->instance = instance;
    render_context->physical_device = physical_device;
    render_context->device = device;

    render_context->swapchain = swapchain;
    render_context->swapchain_image_count = swapchain_image_count;

    render_context->framebuffers = framebuffers;

    render_context->renderpass = renderpass;
    render_context->pipeline = graphics_pipeline;
    render_context->pipeline_layout = pipeline_layout;

    render_context->ready_to_render_semaphores = ready_to_render_semaphores;
    render_context->ready_to_present_semaphores = ready_to_present_semaphores;

    render_context->ready_to_render_fences = ready_to_render_fences;
    render_context->ready_to_present_fences = ready_to_present_fences;

    render_context->graphics_command_buffers = graphics_command_buffers;

    render_context->uniform_buffer_alignment = uniform_buffer_alignment;

    render_context->graphics_queue = graphics_queue;
    //VkQueue transfer_queue;
    //VkQueue present_queue;

    render_context->current_image_index = 0;

    render_context->uniform_buffer = vk_create_uniform_buffer(physical_device, device, 
                                                            sizeof(Uniform), swapchain_image_count, uniform_buffer_alignment);
}
#endif

f32 cube_vertices[] = 
{
    0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,
    0.5f, -0.5f, 0.5f,
    -0.5f, -0.5f, 0.5f, 

    0.5f, 0.5f, -0.5f,
    -0.5f, 0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f, 
};

u32 cube_outward_facing_indices[]
{
    //+z
    0, 1, 2,
    1, 3, 2,

    //-z
    4, 6, 5, 
    5, 6, 7, 

    //+x
    0, 2, 6,
    0, 6, 4,

    //-x
    1, 5, 3,
    5, 7, 3,

    //+y
    5, 0, 4,
    5, 1, 0,

    //-y
    3, 7, 2,
    7, 6, 2
};

u32 cube_inward_facing_indices[]
{
    //+z
    0, 2, 1,
    1, 2, 3,

    //-z
    4, 5, 6, 
    5, 7, 6, 

    //+x
    0, 6, 2,
    0, 4, 6,

    //-x
    1, 3, 5,
    5, 3, 7,

    //+y
    5, 4, 0,
    5, 0, 1,

    //-y
    3, 2, 7,
    7, 2, 6
};




// TODO(joon) Later, we can make this to also 'stream' the meshes(just like the other assets), and put them inside the render mesh
// so that the graphics API can render them.
internal void
metal_render_and_display(MetalRenderContext *render_context, u8 *push_buffer_base, u32 push_buffer_used, u32 window_width, u32 window_height)
{
    // NOTE(joon): renderpass descriptor is already configured for this frame
    MTLRenderPassDescriptor *this_frame_descriptor = render_context->view.currentRenderPassDescriptor;

    //renderpass_descriptor.colorAttachments[0].texture = ;
    this_frame_descriptor.colorAttachments[0].clearColor = {0, 0, 0, 1};
    this_frame_descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    this_frame_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

    this_frame_descriptor.depthAttachment.clearDepth = 1.0f;
    this_frame_descriptor.depthAttachment.loadAction = MTLLoadActionClear;
    this_frame_descriptor.depthAttachment.storeAction = MTLStoreActionDontCare;

    if(this_frame_descriptor)
    {
        id<MTLCommandBuffer> command_buffer = [render_context->command_queue commandBuffer];
        // TODO(joon) double check whether this thing is freed automatically or not
        // if not, we can pull this outside, and put this inside the render context
        id<MTLRenderCommandEncoder> render_encoder = [command_buffer renderCommandEncoderWithDescriptor: this_frame_descriptor];

        metal_set_viewport(render_encoder, 0, 0, window_width, window_height, 0, 1);
        metal_set_scissor_rect(render_encoder, 0, 0, window_width, window_height);
        metal_set_triangle_fill_mode(render_encoder, MTLTriangleFillModeFill);
        metal_set_front_facing_winding(render_encoder, MTLWindingCounterClockwise);
        metal_set_cull_mode(render_encoder, MTLCullModeBack);
        metal_set_detph_stencil_state(render_encoder, render_context->depth_state);

        // TODO(joon) pass the matrices, not the camera itself!
        Camera *camera = (Camera *)(push_buffer_base);
        f32 width_over_height = (f32)window_width / (f32)window_height;
        m4 proj = projection(camera->focal_length, width_over_height, 0.1f, 10000.0f);
        m4 view = world_to_camera(camera->p, 
                camera->initial_x_axis, camera->along_x, 
                camera->initial_y_axis, camera->along_y, 
                camera->initial_z_axis, camera->along_z);

        PerFrameData per_frame_data = {};
        per_frame_data.proj_view_ = proj*camera_rhs_to_lhs(view);

        metal_set_vertex_bytes(render_encoder, &per_frame_data, sizeof(per_frame_data), 1);

        u32 voxel_instance_count = 0;
        for(u32 consumed = sizeof(Camera); // NOTE(joon) The first element of the push buffer is camera
                consumed < push_buffer_used;
                )
        {
            RenderEntryHeader *header = (RenderEntryHeader *)((u8 *)push_buffer_base + consumed);

            switch(header->type)
            {
                // TODO(joon) we can also do the similar thing as the voxels,
                // which is allocating the managed buffer and instance-drawing the lines
                case Render_Entry_Type_Line:
                {
                    RenderEntryLine *entry = (RenderEntryLine *)((u8 *)push_buffer_base + consumed);
                    metal_set_pipeline(render_encoder, render_context->line_pipeline_state);
                    f32 start_and_end[6] = {entry->start.x, entry->start.y, entry->start.z, entry->end.x, entry->end.y, entry->end.z};

                    metal_set_vertex_bytes(render_encoder, start_and_end, sizeof(f32) * array_count(start_and_end), 0);
                    metal_set_vertex_bytes(render_encoder, &entry->color, sizeof(entry->color), 2);

                    metal_draw_non_indexed(render_encoder, MTLPrimitiveTypeLine, 0, 2);

                    consumed += sizeof(*entry);
                }break;
#if 0
                case Render_Entry_Type_Voxel:
                {
                    RenderEntryVoxel *entry = (RenderEntryVoxel *)((u8 *)push_buffer_base + consumed);

                    metal_append_to_managed_buffer(&render_context->voxel_position_buffer, &entry->p, sizeof(entry->p));
                    metal_append_to_managed_buffer(&render_context->voxel_color_buffer, &entry->color, sizeof(entry->color));

                    voxel_instance_count++;

                    consumed += sizeof(*entry);
                }break;
#endif

                case Render_Entry_Type_AABB:
                {
                    RenderEntryAABB *entry = (RenderEntryAABB *)((u8 *)push_buffer_base + consumed);
                    consumed += sizeof(*entry);

                    m4 model = scale_translate(entry->dim, entry->p);
                    PerObjectData per_object_data = {};
                    per_object_data.model_ = model;
                    per_object_data.color_ = entry->color;

                    metal_set_pipeline(render_encoder, render_context->cube_pipeline_state);
                    metal_set_vertex_bytes(render_encoder, cube_vertices, sizeof(f32) * array_count(cube_vertices), 0);
                    metal_set_vertex_bytes(render_encoder, &per_object_data, sizeof(per_object_data), 2);

                    metal_draw_indexed_instances(render_encoder, MTLPrimitiveTypeTriangle, 
                            render_context->cube_inward_facing_index_buffer.buffer, array_count(cube_inward_facing_indices), 1);
                }break;
            }
        }

        // NOTE(joon) draw axis lines
        // TODO(joon) maybe it's more wise to pull the line into seperate entry, and 
        // instance draw them just by the position buffer
        metal_set_pipeline(render_encoder, render_context->line_pipeline_state);

        f32 x_axis[] = {0.0f, 0.0f, 0.0f, 100.0f, 0.0f, 0.0f};
        v3 x_axis_color = V3(1, 0, 0);
        f32 y_axis[] = {0.0f, 0.0f, 0.0f, 0.0f, 100.0f, 0.0f};
        v3 y_axis_color = V3(0, 1, 0);
        f32 z_axis[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 100.0f};
        v3 z_axis_color = V3(0, 0, 1);

        // x axis
        metal_set_vertex_bytes(render_encoder, x_axis, sizeof(f32) * array_count(x_axis), 0);
        metal_set_vertex_bytes(render_encoder, &x_axis_color, sizeof(v3), 2);
        metal_draw_non_indexed(render_encoder, MTLPrimitiveTypeLine, 0, 2);

        // y axis
        metal_set_vertex_bytes(render_encoder, y_axis, sizeof(f32) * array_count(y_axis), 0);
        metal_set_vertex_bytes(render_encoder, &y_axis_color, sizeof(v3), 2);
        metal_draw_non_indexed(render_encoder, MTLPrimitiveTypeLine, 0, 2);

        // z axis
        metal_set_vertex_bytes(render_encoder, z_axis, sizeof(f32) * array_count(z_axis), 0);
        metal_set_vertex_bytes(render_encoder, &z_axis_color, sizeof(v3), 2);
        metal_draw_non_indexed(render_encoder, MTLPrimitiveTypeLine, 0, 2);

        if(voxel_instance_count)
        {
            // NOTE(joon) as we are drawing a lot of voxels, we are going to treat the voxels in a special way by
            // using the instancing.
            metal_flush_managed_buffer(&render_context->voxel_position_buffer);
            metal_flush_managed_buffer(&render_context->voxel_color_buffer);

            metal_set_pipeline(render_encoder, render_context->voxel_pipeline_state);
            metal_set_vertex_bytes(render_encoder, cube_vertices, sizeof(f32) * array_count(cube_vertices), 0);
            metal_set_vertex_buffer(render_encoder, render_context->voxel_position_buffer.buffer, 0, 2);
            metal_set_vertex_buffer(render_encoder, render_context->voxel_color_buffer.buffer, 0, 3);

            //metal_draw_primitives(render_encoder, MTLPrimitiveTypeTriangle, 0, array_count(voxel_vertices)/3, 0, voxel_count);

            metal_draw_indexed_instances(render_encoder, MTLPrimitiveTypeTriangle, 
                    render_context->cube_outward_facing_index_buffer.buffer, array_count(cube_outward_facing_indices), voxel_instance_count);
        }
#if 0
- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType 
                   indexCount:(NSUInteger)indexCount 
                    indexType:(MTLIndexType)indexType 
                  indexBuffer:(id<MTLBuffer>)indexBuffer 
            indexBufferOffset:(NSUInteger)indexBufferOffset 
                instanceCount:(NSUInteger)instanceCount;
#endif


        metal_end_encoding(render_encoder);

        metal_present_drawable(command_buffer, render_context->view);

        // TODO(joon): Sync with the swap buffer!
        metal_commit_command_buffer(command_buffer, render_context->view);
    }
}

// NOTE(joon): returns the base path where all the folders(code, misc, data) are located
internal void
macos_get_base_path(char *dest)
{
    NSString *app_path_string = [[NSBundle mainBundle] bundlePath];
    u32 length = [app_path_string lengthOfBytesUsingEncoding: NSUTF8StringEncoding];
    unsafe_string_append(dest, 
                        [app_path_string cStringUsingEncoding: NSUTF8StringEncoding],
                        length);

    u32 slash_to_delete_count = 2;
    for(u32 index = length-1;
            index >= 0;
            --index)
    {
        if(dest[index] == '/')
        {
            slash_to_delete_count--;
            if(slash_to_delete_count == 0)
            {
                break;
            }
        }
        else
        {
            dest[index] = 0;
        }
    }
}

int 
main(void)
{ 
    struct mach_timebase_info mach_time_info;
    mach_timebase_info(&mach_time_info);
    f32 nano_seconds_per_tick = ((f32)mach_time_info.numer/(f32)mach_time_info.denom);

    u32 random_seed = time(NULL);
    random_series series = start_random_series(random_seed); 

    //TODO : writefile?
    PlatformAPI platform_api = {};
    platform_api.read_file = debug_macos_read_file;
    platform_api.write_entire_file = debug_macos_write_entire_file;
    platform_api.free_file_memory = debug_macos_free_file_memory;

    PlatformMemory platform_memory = {};

    platform_memory.permanent_memory_size = gigabytes(1);
    platform_memory.transient_memory_size = gigabytes(3);
    u64 total_size = platform_memory.permanent_memory_size + platform_memory.transient_memory_size;
    vm_allocate(mach_task_self(), 
                (vm_address_t *)&platform_memory.permanent_memory,
                total_size, 
                VM_FLAGS_ANYWHERE);
    platform_memory.transient_memory = (u8 *)platform_memory.permanent_memory + platform_memory.permanent_memory_size;

    PlatformReadFileResult font = debug_macos_read_file("/Users/mekalopo/Library/Fonts/InputMonoCompressed-Light.ttf");

    i32 window_width = 1920;
    i32 window_height = 1080;

    u32 target_frames_per_second = 60;
    f32 target_seconds_per_frame = 1.0f/(f32)target_frames_per_second;
    u32 target_nano_seconds_per_frame = (u32)(target_seconds_per_frame*sec_to_nano_sec);
    NSApplication *app = [NSApplication sharedApplication];
    [app setActivationPolicy :NSApplicationActivationPolicyRegular];
    app_delegate *delegate = [app_delegate new];
    [app setDelegate: delegate];

    NSMenu *app_main_menu = [NSMenu alloc];
    NSMenuItem *menu_item_with_item_name = [NSMenuItem new];
    [app_main_menu addItem : menu_item_with_item_name];
    [NSApp setMainMenu:app_main_menu];

    NSMenu *SubMenuOfMenuItemWithAppName = [NSMenu alloc];
    NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" 
                                                    action:@selector(terminate:)  // Decides what will happen when the menu is clicked or selected
                                                    keyEquivalent:@"q"];
    [SubMenuOfMenuItemWithAppName addItem:quitMenuItem];
    [menu_item_with_item_name setSubmenu:SubMenuOfMenuItemWithAppName];

    // TODO(joon): when connected to the external display, this should be window_width and window_height
    // but if not, this should be window_width/2 and window_height/2. Why?
    //NSRect window_rect = NSMakeRect(100.0f, 100.0f, (f32)window_width, (f32)window_height);
    NSRect window_rect = NSMakeRect(100.0f, 100.0f, (f32)window_width/2.0f, (f32)window_height/2.0f);

    NSWindow *window = [[NSWindow alloc] initWithContentRect : window_rect
                                        // Apple window styles : https://developer.apple.com/documentation/appkit/nswindow/stylemask
                                        styleMask : NSTitledWindowMask|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable
                                        backing : NSBackingStoreBuffered
                                        defer : NO];

    NSString *app_name = [[NSProcessInfo processInfo] processName];
    [window setTitle:app_name];
    [window makeKeyAndOrderFront:0];
    [window makeKeyWindow];
    [window makeMainWindow];

    char base_path[256] = {};
    macos_get_base_path(base_path);

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    NSString *name = device.name;
    bool has_unified_memory = device.hasUnifiedMemory;

    MTKView *view = [[MTKView alloc] initWithFrame : window_rect
                                        device:device];
    CAMetalLayer *metal_layer = (CAMetalLayer*)[view layer];

    // load vkGetInstanceProcAddr
    //macos_initialize_vulkan(&render_context, metal_layer);

    [window setContentView:view];
    view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;

    MTLDepthStencilDescriptor *depth_descriptor = [MTLDepthStencilDescriptor new];
    depth_descriptor.depthCompareFunction = MTLCompareFunctionLessEqual;
    depth_descriptor.depthWriteEnabled = true;
    id<MTLDepthStencilState> depth_state = [device newDepthStencilStateWithDescriptor:depth_descriptor];
    [depth_descriptor release];

    NSError *error;
    // TODO(joon) : Put the metallib file inside the app
    char metallib_path[256] = {};
    unsafe_string_append(metallib_path, base_path);
    unsafe_string_append(metallib_path, "code/shader/shader.metallib");

    // TODO(joon) : maybe just use newDefaultLibrary? If so, figure out where should we put the .metal files
    id<MTLLibrary> shader_library = [device newLibraryWithFile:[[NSString alloc] initWithCString:metallib_path
                                                                                    encoding:NSUTF8StringEncoding] 
                                                                error: &error];
    check_ns_error(error);

    id<MTLFunction> voxel_vertex = [shader_library newFunctionWithName:@"voxel_vertex"];
    id<MTLFunction> voxel_frag = [shader_library newFunctionWithName:@"voxel_frag"];
    MTLRenderPipelineDescriptor *voxel_pipeline_descriptor = [MTLRenderPipelineDescriptor new];
    voxel_pipeline_descriptor.label = @"Voxel Pipeline";
    voxel_pipeline_descriptor.vertexFunction = voxel_vertex;
    voxel_pipeline_descriptor.fragmentFunction = voxel_frag;
    voxel_pipeline_descriptor.sampleCount = 1;
    voxel_pipeline_descriptor.rasterSampleCount = voxel_pipeline_descriptor.sampleCount;
    voxel_pipeline_descriptor.rasterizationEnabled = true;
    voxel_pipeline_descriptor.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;
    voxel_pipeline_descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    voxel_pipeline_descriptor.colorAttachments[0].writeMask = MTLColorWriteMaskAll;
    voxel_pipeline_descriptor.depthAttachmentPixelFormat = view.depthStencilPixelFormat;

    id<MTLRenderPipelineState> voxel_pipeline_state = [device newRenderPipelineStateWithDescriptor:voxel_pipeline_descriptor
                                                                error:&error];

    id<MTLFunction> cube_vertex = [shader_library newFunctionWithName:@"cube_vertex"];
    id<MTLFunction> cube_frag = [shader_library newFunctionWithName:@"cube_frag"];
    MTLRenderPipelineDescriptor *cube_pipeline_descriptor = [MTLRenderPipelineDescriptor new];
    cube_pipeline_descriptor.label = @"Cube Pipeline";
    cube_pipeline_descriptor.vertexFunction = cube_vertex;
    cube_pipeline_descriptor.fragmentFunction = cube_frag;
    cube_pipeline_descriptor.sampleCount = 1;
    cube_pipeline_descriptor.rasterSampleCount = cube_pipeline_descriptor.sampleCount;
    cube_pipeline_descriptor.rasterizationEnabled = true;
    cube_pipeline_descriptor.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;
    cube_pipeline_descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    cube_pipeline_descriptor.colorAttachments[0].writeMask = MTLColorWriteMaskAll;
    cube_pipeline_descriptor.depthAttachmentPixelFormat = view.depthStencilPixelFormat;

    id<MTLRenderPipelineState> cube_pipeline_state = [device newRenderPipelineStateWithDescriptor:cube_pipeline_descriptor
                                                                error:&error];

    id<MTLFunction> line_vertex = [shader_library newFunctionWithName:@"line_vertex"];
    id<MTLFunction> line_frag = [shader_library newFunctionWithName:@"line_frag"];
    MTLRenderPipelineDescriptor *line_pipeline_descriptor = [MTLRenderPipelineDescriptor new];
    line_pipeline_descriptor.label = @"Line Pipeline";
    line_pipeline_descriptor.vertexFunction = line_vertex;
    line_pipeline_descriptor.fragmentFunction = line_frag;
    line_pipeline_descriptor.sampleCount = 1;
    line_pipeline_descriptor.rasterSampleCount = line_pipeline_descriptor.sampleCount;
    line_pipeline_descriptor.rasterizationEnabled = true;
    line_pipeline_descriptor.inputPrimitiveTopology = MTLPrimitiveTopologyClassLine;
    line_pipeline_descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    line_pipeline_descriptor.colorAttachments[0].writeMask = MTLColorWriteMaskAll;
    line_pipeline_descriptor.depthAttachmentPixelFormat = view.depthStencilPixelFormat;

    id<MTLRenderPipelineState> line_pipeline_state = [device newRenderPipelineStateWithDescriptor:line_pipeline_descriptor
                                                                error:&error];

    check_ns_error(error);

    id<MTLCommandQueue> command_queue = [device newCommandQueue];

    MetalRenderContext metal_render_context = {};
    metal_render_context.device = device;
    metal_render_context.view = view;
    metal_render_context.command_queue = command_queue;
    metal_render_context.depth_state = depth_state;
    metal_render_context.voxel_pipeline_state = voxel_pipeline_state;
    metal_render_context.cube_pipeline_state = cube_pipeline_state;
    metal_render_context.line_pipeline_state = line_pipeline_state;
    // TODO(joon) More robust way to manage these buffers??(i.e asset system?)
    metal_render_context.voxel_position_buffer = metal_create_managed_buffer(device, megabytes(16));
    metal_render_context.voxel_color_buffer = metal_create_managed_buffer(device, megabytes(4));
    metal_render_context.cube_outward_facing_index_buffer = metal_create_managed_buffer(device, sizeof(u32) * array_count(cube_outward_facing_indices));
    metal_append_to_managed_buffer(&metal_render_context.cube_outward_facing_index_buffer, 
                                    cube_outward_facing_indices, 
                                    metal_render_context.cube_outward_facing_index_buffer.max_size);
    metal_render_context.cube_inward_facing_index_buffer = metal_create_managed_buffer(device, sizeof(u32) * array_count(cube_inward_facing_indices));
    metal_append_to_managed_buffer(&metal_render_context.cube_inward_facing_index_buffer, 
                                    cube_inward_facing_indices, 
                                    metal_render_context.cube_inward_facing_index_buffer.max_size);

    CVDisplayLinkRef display_link;
    if(CVDisplayLinkCreateWithActiveCGDisplays(&display_link)== kCVReturnSuccess)
    {
        CVDisplayLinkSetOutputCallback(display_link, display_link_callback, 0); 
        CVDisplayLinkStart(display_link);
    }

    PlatformInput platform_input = {};
    u32 render_push_buffer_max_size = megabytes(16);
    u8 *render_push_buffer_base = (u8 *)malloc(render_push_buffer_max_size);
    u32 render_push_buffer_used = 0;

    [app activateIgnoringOtherApps:YES];
    [app run];

    u64 last_time = mach_absolute_time();
    is_game_running = true;
    while(is_game_running)
    {
        platform_input.dt_per_frame = target_seconds_per_frame;
        macos_handle_event(app, window, &platform_input);

        // TODO(joon): check if the focued window is working properly
        b32 is_window_focused = [app keyWindow] && [app mainWindow];

        /*
            TODO(joon) : For more precisely timed rendering, the operations should be done in this order
            1. Update the game based on the input
            2. Check the mach absolute time
            3. With the return value from the displayLinkOutputCallback function, get the absolute time to present
            4. Use presentDrawable:atTime to present at the specific time
        */
        update_and_render(&platform_api, &platform_input, &platform_memory, render_push_buffer_base, render_push_buffer_max_size, &render_push_buffer_used);

        u64 time_passed_in_nano_seconds = mach_time_diff_in_nano_seconds(last_time, mach_absolute_time(), nano_seconds_per_tick);

        // NOTE(joon): Because nanosleep is such a high resolution sleep method, for precise timing,
        // we need to undersleep and spend time in a loop
        u64 undersleep_nano_seconds = 100000000;
        if(time_passed_in_nano_seconds < target_nano_seconds_per_frame)
        {
            timespec time_spec = {};
            time_spec.tv_nsec = target_nano_seconds_per_frame - time_passed_in_nano_seconds -  undersleep_nano_seconds;

            nanosleep(&time_spec, 0);
        }
        else
        {
            // TODO : Missed Frame!
            // TODO(joon) : Whenever we miss the frame re-sync with the display link
        }

        // For a short period of time, loop
        time_passed_in_nano_seconds = mach_time_diff_in_nano_seconds(last_time, mach_absolute_time(), nano_seconds_per_tick);
        while(time_passed_in_nano_seconds < target_nano_seconds_per_frame)
        {
            time_passed_in_nano_seconds = mach_time_diff_in_nano_seconds(last_time, mach_absolute_time(), nano_seconds_per_tick);
        }
        u32 time_passed_in_micro_sec = (u32)(time_passed_in_nano_seconds / 1000);
        f32 time_passed_in_sec = (f32)time_passed_in_micro_sec / 1000000.0f;
        printf("%dms elapsed, fps : %.6f\n", time_passed_in_micro_sec, 1.0f/time_passed_in_sec);
        @autoreleasepool
        {
            metal_render_and_display(&metal_render_context, render_push_buffer_base, render_push_buffer_used, window_width, window_height);
        }

#if 0
        // NOTE(joon) : debug_printf_all_cycle_counters
        for(u32 cycle_counter_index = 0;
                cycle_counter_index < debug_cycle_counter_count;
                cycle_counter_index++)
        {
            printf("ID:%u  Total Cycles: %llu Hit Count: %u, CyclesPerHit: %u\n", cycle_counter_index, 
                                                                             debug_cycle_counters[cycle_counter_index].cycle_count,
                                                                            debug_cycle_counters[cycle_counter_index].hit_count, 
                                                                            (u32)(debug_cycle_counters[cycle_counter_index].cycle_count/debug_cycle_counters[cycle_counter_index].hit_count));
        }
#endif

        // update the time stamp
        last_time = mach_absolute_time();
        // TODO(joon) not loving this...
        render_push_buffer_used = 0;
    }

    return 0;
}











