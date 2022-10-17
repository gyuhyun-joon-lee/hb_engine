/*
 * Written by Gyuhyun Lee
 */

#include <Cocoa/Cocoa.h> 
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

// TODO(gh) introspection?
#undef internal
#undef assert

// TODO(gh) shared.h file for files that are shared across platforms?
#include "hb_types.h"
#include "hb_intrinsic.h"
#include "hb_math.h"
#include "hb_random.h"
#include "hb_simd.h"
#include "hb_render_group.h"
#include "hb_shared_with_shader.h"
#include "hb_platform.h"

#include "hb_metal.cpp"

// TODO(gh): Get rid of global variables?
global v2 last_mouse_p;
global v2 mouse_diff;

global b32 is_game_running;
global dispatch_semaphore_t semaphore;

// TODO(gh) temporary thing to remove render_group.cpp
internal m4x4 
camera_transform(v3 camera_p, v3 camera_x_axis, v3 camera_y_axis, v3 camera_z_axis)
{
    m4x4 result = {};

    // NOTE(gh) to pack the rotation & translation into one matrix(with an order of translation and the rotation),
    // we need to first multiply the translation by the rotation matrix
    v3 multiplied_translation = V3(dot(camera_x_axis, -camera_p), 
                                    dot(camera_y_axis, -camera_p),
                                    dot(camera_z_axis, -camera_p));

    result.rows[0] = V4(camera_x_axis, multiplied_translation.x);
    result.rows[1] = V4(camera_y_axis, multiplied_translation.y);
    result.rows[2] = V4(camera_z_axis, multiplied_translation.z);
    result.rows[3] = V4(0.0f, 0.0f, 0.0f, 1.0f); // Dont need to touch the w part, view matrix doesn't produce homogeneous coordinates

    return result;
}

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
            // TODO/gh : no more os level allocations!
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
            // TODO(gh) : log
        }

        close(file);
    }
    else
    {
        // TODO(gh) :log
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

internal CVReturn 
display_link_callback(CVDisplayLinkRef displayLink, const CVTimeStamp* current_time, const CVTimeStamp* output_time,
                CVOptionFlags ignored_0, CVOptionFlags* ignored_1, void* displayLinkContext)
{
    local_persist u64 last_time = 0;
    u64 time_passed_in_nsec = clock_gettime_nsec_np(CLOCK_UPTIME_RAW) - last_time;
    u64 time_until_output = output_time->hostTime - last_time;

    // printf("%lldns time passed, %lldns until output\n", time_passed_in_nsec, time_until_output);

    last_time = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
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

    // NOTE(gh) : MacOS screen coordinate is bottom-up, so just for the convenience, make y to be bottom-up
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

// TODO(gh) : It seems like this combines read & write barrier, but make sure
// TODO(gh) : mfence?(DSB)
#define write_barrier() OSMemoryBarrier(); 
#define read_barrier() OSMemoryBarrier();

struct MacOSThread
{
    u32 ID;
    ThreadWorkQueue *queue;

    // TODO(gh): I like the idea of each thread having a random number generator that they can use throughout the whole process
    // though what should happen to the 0th thread(which does not have this structure)?
    simd_random_series series;
};

// NOTE(gh) : use this to add what thread should do
internal 
THREAD_WORK_CALLBACK(print_string)
{
    char *stringToPrint = (char *)data;
    printf("%s\n", stringToPrint);
}

// NOTE(gh): This is single producer multiple consumer - 
// meaning, it _does not_ provide any thread safety
// For example, if the two threads try to add the work item,
// one item might end up over-writing the other one
internal
PLATFORM_ADD_THREAD_WORK_QUEUE_ITEM(macos_add_thread_work_item) 
{
    u32 next_add_item_index = (queue->add_index+1) % array_count(queue->items);
    assert(queue->work_index != next_add_item_index);

    assert(data); // TODO(gh) : There might be a work that does not need any data?
    ThreadWorkItem *item = queue->items + queue->add_index;
    item->callback = thread_work_callback;
    item->data = data;
    item->written = true;

    queue->add_index = next_add_item_index;
    queue->completion_goal++;
    write_barrier();

    // increment the semaphore value by 1
    dispatch_semaphore_signal(semaphore);
}

internal b32
macos_do_thread_work_item(ThreadWorkQueue *queue, u32 thread_index)
{
    b32 did_work = false;
    int original_work_index = queue->work_index;
    int desired_work_index = (original_work_index + 1) % array_count(queue->items);

    // Should check this for later, because some other thread might have figgled with work_index
    if(original_work_index != queue->add_index)
    {
        if(OSAtomicCompareAndSwapIntBarrier(original_work_index, desired_work_index, &queue->work_index)) // old, new, ptr
        {
            ThreadWorkItem *item = queue->items + original_work_index;
            item->callback(item->data);
            OSAtomicIncrement32Barrier(&queue->completion_count);

            //printf("Thread %u: Finished working\n", thread_index);
            did_work = true;
        }
    }

    return did_work;
}

internal 
PLATFORM_COMPLETE_ALL_THREAD_WORK_QUEUE_ITEMS(macos_complete_all_thread_work_queue_items)
{
    while(queue->completion_count != queue->completion_goal)
    {
        macos_do_thread_work_item(queue, 0);
    }

    queue->completion_count = 0;
    queue->completion_goal = 0;
}

internal void*
thread_proc(void *data)
{
    MacOSThread *thread = (MacOSThread *)data;
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

// TODO(gh) Later, we can make this to also 'stream' the meshes(just like the other assets), 
// and put them inside the render mesh so that the graphics API can render them.
internal void
metal_render_and_display(MetalRenderContext *render_context, PlatformRenderPushBuffer *render_push_buffer, u32 window_width, u32 window_height, f32 time_elasped_from_start)
{
    id<MTLCommandBuffer> shadow_command_buffer = [render_context->command_queue commandBuffer];

    // NOTE(gh) render shadow map
    id<MTLRenderCommandEncoder> shadowmap_render_encoder = [shadow_command_buffer renderCommandEncoderWithDescriptor : render_context->directional_light_shadowmap_renderpass];
    shadowmap_render_encoder.label = @"Shadowmap Render";
    metal_set_viewport(shadowmap_render_encoder, 0, 0, 
                       render_context->directional_light_shadowmap_depth_texture_width, 
                       render_context->directional_light_shadowmap_depth_texture_height, 
                       0, 1); // TODO(gh) near and far value when rendering the shadowmap)
    metal_set_scissor_rect(shadowmap_render_encoder, 0, 0, 
                           render_context->directional_light_shadowmap_depth_texture_width, 
                           render_context->directional_light_shadowmap_depth_texture_height);
    metal_set_triangle_fill_mode(shadowmap_render_encoder, MTLTriangleFillModeFill);
    metal_set_front_facing_winding(shadowmap_render_encoder, MTLWindingCounterClockwise);
    // Culling front facing triangles when rendering shadowmap to avoid 
    // shadow acne(moire pattern in non-shaded sides). This effectively 'biases' the shadowmap value down
    // TODO(gh) This does not work for thin objects!!!!
    metal_set_cull_mode(shadowmap_render_encoder, MTLCullModeFront); 
    metal_set_depth_stencil_state(shadowmap_render_encoder, render_context->depth_state);

    metal_set_render_pipeline(shadowmap_render_encoder, render_context->directional_light_shadowmap_pipeline);

    local_persist f32 rad = 2.4f;
    v3 directional_light_p = V3(3 * cosf(rad), 3 * sinf(rad), 100); 
    v3 directional_light_direction = normalize(-directional_light_p); // This will be our -Z in camera for the shadowmap

    // TODO(gh) These are totally made up near, far, width values 
    // m4x4 light_proj = perspective_projection(degree_to_radian(120), 0.01f, 100.0f, (f32)render_context->directional_light_shadowmap_depth_texture_width / (f32)render_context->directional_light_shadowmap_depth_texture_height);
    m4x4 light_proj = orthogonal_projection(0.01f, 10000.0f, 50.0f, render_context->directional_light_shadowmap_depth_texture_width / (f32)render_context->directional_light_shadowmap_depth_texture_height);
    v3 directional_light_z_axis = -directional_light_direction;
    v3 directional_light_x_axis = normalize(cross(V3(0, 0, 1), directional_light_z_axis));
    v3 directional_light_y_axis = normalize(cross(directional_light_z_axis, directional_light_x_axis));
    m4x4 light_view = camera_transform(directional_light_p, 
                                       directional_light_x_axis, 
                                       directional_light_y_axis, 
                                       directional_light_z_axis);
    m4x4 light_proj_view = transpose(light_proj * light_view); // Change to column major

    // NOTE(gh) Render Shadow map
    for(u32 consumed = 0;
            consumed < render_push_buffer->used;
            )
    {
        RenderEntryHeader *header = (RenderEntryHeader *)((u8 *)render_push_buffer->base + consumed);

        switch(header->type)
        {
            case RenderEntryType_AABB:
            {
                RenderEntryAABB *entry = (RenderEntryAABB *)((u8 *)render_push_buffer->base + consumed);
                consumed += sizeof(*entry);

                if(entry->should_cast_shadow)
                {
                    m4x4 model = M4x4();
                    model = transpose(model); // make the matrix column-major

                    metal_set_vertex_buffer(shadowmap_render_encoder, render_context->combined_vertex_buffer.buffer, entry->vertex_buffer_offset, 0);
                    metal_set_vertex_bytes(shadowmap_render_encoder, &model, sizeof(model), 1);
                    metal_set_vertex_bytes(shadowmap_render_encoder, &light_proj_view, sizeof(light_proj_view), 2);

                    // NOTE(gh) Mitigates the moire pattern by biasing, 
                    // making the shadow map to place under the fragments that are being shaded.
                    // metal_set_depth_bias(shadowmap_render_encoder, 0.015f, 7, 0.02f);

                    metal_draw_indexed(shadowmap_render_encoder, MTLPrimitiveTypeTriangle, 
                                      render_context->combined_index_buffer.buffer, entry->index_buffer_offset, entry->index_count);
                }
            }break;

            case RenderEntryType_Cube:
            {
                RenderEntryCube *entry = (RenderEntryCube *)((u8 *)render_push_buffer->base + consumed);
                consumed += sizeof(*entry);

                if(entry->should_cast_shadow)
                {
                    m4x4 model = st_m4x4(entry->p, entry->dim);
                    model = transpose(model); // make the matrix column-major

                    metal_set_vertex_buffer(shadowmap_render_encoder, render_context->combined_vertex_buffer.buffer, entry->vertex_buffer_offset, 0);
                    metal_set_vertex_bytes(shadowmap_render_encoder, &model, sizeof(model), 1);
                    metal_set_vertex_bytes(shadowmap_render_encoder, &light_proj_view, sizeof(light_proj_view), 2);

                    // NOTE(gh) Mitigates the moire pattern by biasing, 
                    // making the shadow map to place under the fragments that are being shaded.
                    // metal_set_depth_bias(shadowmap_render_encoder, 0.015f, 7, 0.02f);

                    metal_draw_indexed(shadowmap_render_encoder, MTLPrimitiveTypeTriangle, 
                                      render_context->combined_index_buffer.buffer, entry->index_buffer_offset, entry->index_count);
                }
            }break;

            default: 
            {
                consumed += header->size;
            };
        }
    }

    metal_end_encoding(shadowmap_render_encoder);
    // We can start working on things that don't require drawable_texture.
    metal_commit_command_buffer(shadow_command_buffer);

    // TODO(gh) Do we need to check whether the shadowmap was fully rendered?

    id<MTLCommandBuffer> command_buffer = [render_context->command_queue commandBuffer];


    id<MTLRenderCommandEncoder> clear_g_buffer_render_encoder = 
        [command_buffer renderCommandEncoderWithDescriptor: render_context->clear_g_buffer_renderpass];
    clear_g_buffer_render_encoder.label = @"Clear G Buffers";
    metal_end_encoding(clear_g_buffer_render_encoder);
    

    // TODO(gh) double check whether this thing is freed automatically or not
    // if not, we can pull this outside, and put this inside the render context
    // NOTE(gh) When we create a render_encoder, we cannot create another render encoder until we call endEncoding on the current one.

    m4x4 render_proj = perspective_projection(
            render_push_buffer->render_camera_fov, 
            render_push_buffer->render_camera_near, 
            render_push_buffer->render_camera_far,
            render_push_buffer->width_over_height);
    m4x4 render_proj_view = transpose(render_proj * render_push_buffer->render_camera_view);

    m4x4 game_proj = perspective_projection(render_push_buffer->game_camera_fov, render_push_buffer->game_camera_near, render_push_buffer->game_camera_far,
                                       render_push_buffer->width_over_height);
    m4x4 game_proj_view = transpose(game_proj * render_push_buffer->game_camera_view);

    PerFrameData per_frame_data = {};
    per_frame_data.proj_view = render_proj_view;

    u32 grid_to_render_count = 0;
    if(render_push_buffer->enable_grass_rendering)
    {
        // init grass count
        // TODO(gh) not sure why, but it seems like this removes the nasty bug that I had before...
        id<MTLComputeCommandEncoder> initialize_grass_counts_encoder = [command_buffer computeCommandEncoder];
        metal_set_compute_pipeline(initialize_grass_counts_encoder, render_context->initialize_grass_counts_pipeline);
        metal_set_compute_buffer(initialize_grass_counts_encoder, render_context->grass_start_count_buffer.buffer, 0, 0);
        metal_set_compute_buffer(initialize_grass_counts_encoder, render_context->grass_count_buffer.buffer, 0, 1);
        metal_dispatch_compute_threads(initialize_grass_counts_encoder, V3u(1, 1, 1), V3u(1, 1, 1));
        metal_memory_barrier_with_scope(initialize_grass_counts_encoder, MTLBarrierScopeBuffers);
        metal_end_encoding(initialize_grass_counts_encoder);

#if 0
        metal_get_timestamp(&render_context->grass_rendering_start_timestamp, command_buffer, render_context->device);
        // NOTE(gh) first, render all the grasses with mesh render pipeline
        metal_set_viewport(render_encoder, 0, 0, window_width, window_height, 0, 1);
        metal_set_scissor_rect(render_encoder, 0, 0, window_width, window_height);
        metal_set_triangle_fill_mode(render_encoder, MTLTriangleFillModeFill);
        metal_set_front_facing_winding(render_encoder, MTLWindingCounterClockwise);
        metal_set_cull_mode(render_encoder, MTLCullModeNone); 
        metal_set_depth_stencil_state(render_encoder, render_context->depth_state);
        metal_set_render_pipeline(render_encoder, render_context->grass_mesh_render_pipeline);

        for(u32 grass_grid_index = 0;
                grass_grid_index < render_push_buffer->grass_grid_count_x * render_push_buffer->grass_grid_count_y;
                ++grass_grid_index)
        {
            GrassGrid *grid = render_push_buffer->grass_grids + grass_grid_index;

            if(grid->should_draw)
            {
                assert(grid->grass_count_x % object_thread_per_threadgroup_count_x == 0);
                assert(grid->grass_count_y % object_thread_per_threadgroup_count_y == 0);
                u32 object_threadgroup_per_grid_count_x = grid->grass_count_x / object_thread_per_threadgroup_count_x;
                u32 object_threadgroup_per_grid_count_y = grid->grass_count_y / object_thread_per_threadgroup_count_y;
                
                v2 grid_dim = grid->max - grid->min;

                GrassObjectFunctionInput grass_object_input = {};
                grass_object_input.min = grid->min;
                grass_object_input.max = grid->max;
                grass_object_input.one_thread_worth_dim = V2(grid_dim.x/grid->grass_count_x, grid_dim.y/grid->grass_count_y);

                metal_set_object_bytes(render_encoder, &grass_object_input, sizeof(grass_object_input), 0);
                metal_set_object_buffer(render_encoder, render_context->giant_buffer.buffer, grid->hash_buffer_offset, 1);
                metal_set_object_buffer(render_encoder, render_context->giant_buffer.buffer, grid->floor_z_buffer_offset, 2);
                metal_set_object_bytes(render_encoder, &time_elasped_from_start, sizeof(time_elasped_from_start), 3);
                metal_set_object_bytes(render_encoder, &game_proj_view, sizeof(game_proj_view), 4);
                metal_set_object_buffer(render_encoder, render_context->giant_buffer.buffer, grid->perlin_noise_buffer_offset, 5);

                metal_set_mesh_bytes(render_encoder, &game_proj_view, sizeof(game_proj_view), 0);
                metal_set_mesh_bytes(render_encoder, &render_proj_view, sizeof(render_proj_view), 1);
                metal_set_mesh_bytes(render_encoder, &light_proj_view, sizeof(light_proj_view), 2);
                metal_set_mesh_bytes(render_encoder, &render_push_buffer->game_camera_p, sizeof(render_push_buffer->game_camera_p), 3);
                metal_set_mesh_bytes(render_encoder, &render_push_buffer->render_camera_p, sizeof(render_push_buffer->render_camera_p), 4);

                metal_set_fragment_sampler(render_encoder, render_context->shadowmap_sampler, 0);
                metal_set_fragment_texture(render_encoder, render_context->directional_light_shadowmap_depth_texture, 0);
                v3u object_threadgroup_per_grid_count = V3u(object_threadgroup_per_grid_count_x, 
                                                            object_threadgroup_per_grid_count_y, 
                                                            1);
                v3u object_thread_per_threadgroup_count = V3u(object_thread_per_threadgroup_count_x, 
                                                            object_thread_per_threadgroup_count_y, 
                                                            1);
                // TODO(gh) use low lod index count for grids further away
                v3u mesh_thread_per_threadgroup_count = V3u(grass_high_lod_index_count, 1, 1); // same as index count for the grass blade

                metal_draw_mesh_thread_groups(render_encoder, object_threadgroup_per_grid_count, 
                        object_thread_per_threadgroup_count, 
                        mesh_thread_per_threadgroup_count);
            }
        }

        metal_get_timestamp(&render_context->grass_rendering_end_timestamp, command_buffer, render_context->device);
        
        assert(sizeof(GrassInstanceData)*render_push_buffer->grass_grid_count_x*render_push_buffer->grass_grid_count_y*grid->grass_count_x*grid->grass_count_y <= 
                render_context->grass_instance_buffer.size);
#endif
        
        u32 total_grid_count = render_push_buffer->grass_grid_count_x * render_push_buffer->grass_grid_count_y;
        for(u32 grass_grid_index = 0;
                grass_grid_index < total_grid_count;
                ++grass_grid_index)
        {
            /*
               If the instance data per grass count is 64 bytes,
               64 * 256 * 256 = 4mb per grid.
             */
            GrassGrid *grid = render_push_buffer->grass_grids + grass_grid_index;
            v2 grid_dim = grid->max - grid->min;
            v2 one_thread_worth_dim = V2(grid_dim.x/grid->grass_count_x, grid_dim.y/grid->grass_count_y);

            if(grid->should_draw)
            {
                u32 wavefront_x = 8;
                u32 wavefront_y = 4;
                assert(grid->grass_count_x % wavefront_x == 0);
                assert(grid->grass_count_y % wavefront_y == 0);

                GridInfo grid_info = {};
                grid_info.min = grid->min;
                grid_info.max = grid->max;
                grid_info.one_thread_worth_dim = one_thread_worth_dim;

                id<MTLComputeCommandEncoder> fill_grass_instance_compute_encoder = [command_buffer computeCommandEncoder];
                NSString *a = @"Fill Grass Instance Data";
                fill_grass_instance_compute_encoder.label = [a stringByAppendingFormat:@"%u", grass_grid_index];
                
                metal_set_compute_pipeline(fill_grass_instance_compute_encoder, render_context->fill_grass_instance_data_pipeline);
                metal_set_compute_buffer(fill_grass_instance_compute_encoder, render_context->grass_count_buffer.buffer, 0, 0);
                // offset is not needed because we are indexing the array using the grass index,
                // which gets accumulated anyway
                metal_set_compute_buffer(fill_grass_instance_compute_encoder, render_context->grass_instance_buffer.buffer, 0, 1);
                metal_set_compute_buffer(fill_grass_instance_compute_encoder, render_context->giant_buffer.buffer, grid->perlin_noise_buffer_offset, 2);
                metal_set_compute_buffer(fill_grass_instance_compute_encoder, render_context->giant_buffer.buffer, grid->floor_z_buffer_offset, 3);
                metal_set_compute_bytes(fill_grass_instance_compute_encoder, &grid_info, sizeof(grid_info), 4);
                metal_set_compute_bytes(fill_grass_instance_compute_encoder, &game_proj_view, sizeof(game_proj_view), 5);

                metal_dispatch_compute_threads(fill_grass_instance_compute_encoder, V3u(grid->grass_count_x, grid->grass_count_y, 1), V3u(wavefront_x, wavefront_y, 1));
                metal_memory_barrier_with_scope(fill_grass_instance_compute_encoder, MTLBarrierScopeBuffers);
                metal_end_encoding(fill_grass_instance_compute_encoder);
                 
                grid_to_render_count++;

                // flush the command every 4 grids or when we ran out of grids
                if((grid_to_render_count % 4 == 0 && (grid_to_render_count != 0)) || (grass_grid_index == total_grid_count-1))
                {
                    // Reset the indirect command buffer
                    id<MTLBlitCommandEncoder> icb_reset_encoder = [command_buffer blitCommandEncoder];
                    if(render_context->next_grass_double_buffer_index == 0)
                    {
                        icb_reset_encoder.label = @"Reset ICB 0";
                    }
                    else
                    {
                        icb_reset_encoder.label = @"Reset ICB 1";
                    }
                    [icb_reset_encoder resetCommandsInBuffer: render_context->icb[render_context->next_grass_double_buffer_index]
                        withRange:NSMakeRange(0, 1)];
                    [icb_reset_encoder endEncoding];

                    id<MTLComputeCommandEncoder> encode_instanced_grass_encoder = [command_buffer computeCommandEncoder];
                    if(render_context->next_grass_double_buffer_index == 0)
                    {
                        encode_instanced_grass_encoder.label = @"Encode Instanced Grass Render Commands 0";
                    }
                    else
                    {
                        encode_instanced_grass_encoder.label = @"Encode Instanced Grass Render Commands 1";
                    }
                    metal_set_compute_pipeline(encode_instanced_grass_encoder, render_context->encode_instanced_grass_render_commands_pipeline);
                    metal_set_compute_buffer(
                            encode_instanced_grass_encoder, 
                            render_context->icb_argument_buffer[render_context->next_grass_double_buffer_index].buffer, 
                            0, 0);
                    metal_set_compute_buffer(encode_instanced_grass_encoder, render_context->grass_start_count_buffer.buffer, 0, 1);
                    metal_set_compute_buffer(encode_instanced_grass_encoder, render_context->grass_count_buffer.buffer, 0, 2);
                    metal_set_compute_buffer(encode_instanced_grass_encoder, render_context->grass_instance_buffer.buffer, 0, 3);
                    metal_set_compute_buffer(encode_instanced_grass_encoder, render_context->grass_index_buffer.buffer, 0, 4);
                    metal_set_compute_bytes(encode_instanced_grass_encoder, &render_proj_view, sizeof(render_proj_view), 5);
                    metal_set_compute_bytes(encode_instanced_grass_encoder, &light_proj_view, sizeof(light_proj_view), 6);
                    metal_set_compute_bytes(encode_instanced_grass_encoder, &render_push_buffer->game_camera_p, sizeof(render_push_buffer->game_camera_p), 7);
                    metal_set_compute_bytes(encode_instanced_grass_encoder, &time_elasped_from_start, sizeof(time_elasped_from_start), 8);

                    // Tell Metal that we are going to write to the indirect command buffer
                    [encode_instanced_grass_encoder useResource:render_context->icb[render_context->next_grass_double_buffer_index] usage:MTLResourceUsageWrite];

                    // TODO(gh) we can combine some of the grids to dispatch render commands, but does that make sense?
                    metal_dispatch_compute_threads(encode_instanced_grass_encoder, V3u(1, 1, 1), V3u(1, 1, 1));
                    metal_end_encoding(encode_instanced_grass_encoder);

                    // Optimize the indirect command buffer
                    metal_optimize_icb(command_buffer, render_context->icb[render_context->next_grass_double_buffer_index]);

                    id<MTLRenderCommandEncoder> g_buffer_render_encoder = 
                        [command_buffer renderCommandEncoderWithDescriptor: render_context->g_buffer_renderpass];
                    NSString *b = @"Fill Grass Instance Data";
                    g_buffer_render_encoder.label = [b stringByAppendingFormat:@"%u", grid_to_render_count];

                    metal_set_viewport(g_buffer_render_encoder, 0, 0, window_width, window_height, 0, 1);
                    metal_set_scissor_rect(g_buffer_render_encoder, 0, 0, window_width, window_height);
                    metal_set_triangle_fill_mode(g_buffer_render_encoder, MTLTriangleFillModeFill);
                    metal_set_front_facing_winding(g_buffer_render_encoder, MTLWindingCounterClockwise);
                    metal_set_cull_mode(g_buffer_render_encoder, MTLCullModeNone); 
                    metal_set_depth_stencil_state(g_buffer_render_encoder, render_context->depth_state);
                    metal_set_render_pipeline(g_buffer_render_encoder, render_context->instanced_grass_render_pipeline);
                    [g_buffer_render_encoder 
                        executeCommandsInBuffer:render_context->icb[render_context->next_grass_double_buffer_index] 
                            withRange:NSMakeRange(0, 1)
                    ];
#if 0
                    // TODO(gh) not sure if we really need this fence?
                    metal_signal_fence_after(
                            g_buffer_render_encoder, 
                            render_context->grass_double_buffer_fence[render_context->next_grass_double_buffer_index],
                            MTLRenderStageVertex);
#endif
                    metal_end_encoding(g_buffer_render_encoder);

                    render_context->next_grass_double_buffer_index = (render_context->next_grass_double_buffer_index+1)%2;
                }
            }
        }
    }

#if 1
    id<MTLRenderCommandEncoder> g_buffer_render_encoder = 
        [command_buffer renderCommandEncoderWithDescriptor: render_context->g_buffer_renderpass];
    g_buffer_render_encoder.label = @"Object G Buffer Rendering";
    metal_set_viewport(g_buffer_render_encoder, 0, 0, window_width, window_height, 0, 1);
    metal_set_scissor_rect(g_buffer_render_encoder, 0, 0, window_width, window_height);
    metal_set_triangle_fill_mode(g_buffer_render_encoder, MTLTriangleFillModeFill);
    metal_set_front_facing_winding(g_buffer_render_encoder, MTLWindingCounterClockwise);
    metal_set_depth_stencil_state(g_buffer_render_encoder, render_context->depth_state);
    metal_set_cull_mode(g_buffer_render_encoder, MTLCullModeBack); 

    for(u32 consumed = 0;
            consumed < render_push_buffer->used;
            )
    {
        RenderEntryHeader *header = (RenderEntryHeader *)((u8 *)render_push_buffer->base + consumed);

        switch(header->type)
        {
            case RenderEntryType_AABB:
            {
                RenderEntryAABB *entry = (RenderEntryAABB *)((u8 *)render_push_buffer->base + consumed);
                consumed += sizeof(*entry);

                m4x4 model = M4x4();
                model = transpose(model); // make the matrix column-major

                PerObjectData per_object_data = {};
                per_object_data.model = model;
                per_object_data.color = entry->color;

                // TODO(gh) Sort the render entry based on cull mode
                // metal_set_cull_mode(g_buffer_render_encoder, MTLCullModeBack); 
                metal_set_render_pipeline(g_buffer_render_encoder, render_context->singlepass_cube_pipeline);
                metal_set_vertex_bytes(g_buffer_render_encoder, &per_frame_data, sizeof(per_frame_data), 0);
                metal_set_vertex_bytes(g_buffer_render_encoder, &per_object_data, sizeof(per_object_data), 1);
                metal_set_vertex_buffer(g_buffer_render_encoder, 
                                        render_context->combined_vertex_buffer.buffer, 
                                        entry->vertex_buffer_offset, 2);
                metal_set_vertex_bytes(g_buffer_render_encoder, &light_proj_view, sizeof(light_proj_view), 3);

                metal_set_fragment_sampler(g_buffer_render_encoder, render_context->shadowmap_sampler, 0);
                metal_set_fragment_texture(g_buffer_render_encoder, render_context->directional_light_shadowmap_depth_texture, 0);

                metal_draw_indexed(g_buffer_render_encoder, MTLPrimitiveTypeTriangle, 
                        render_context->combined_index_buffer.buffer, entry->index_buffer_offset, entry->index_count);
            }break;

            case RenderEntryType_Cube:
            {
                RenderEntryCube *entry = (RenderEntryCube *)((u8 *)render_push_buffer->base + consumed);
                consumed += sizeof(*entry);

                m4x4 model = st_m4x4(entry->p, entry->dim);
                model = transpose(model); // make the matrix column-major

                PerObjectData per_object_data = {};
                per_object_data.model = model;
                per_object_data.color = entry->color;

                // TODO(gh) Sort the render entry based on cull mode
                // metal_set_cull_mode(g_buffer_render_encoder, MTLCullModeBack); 

                metal_set_render_pipeline(g_buffer_render_encoder, render_context->singlepass_cube_pipeline);
                metal_set_vertex_bytes(g_buffer_render_encoder, &per_frame_data, sizeof(per_frame_data), 0);
                metal_set_vertex_bytes(g_buffer_render_encoder, &per_object_data, sizeof(per_object_data), 1);
                metal_set_vertex_buffer(g_buffer_render_encoder, 
                                        render_context->combined_vertex_buffer.buffer, 
                                        entry->vertex_buffer_offset, 2);
                metal_set_vertex_bytes(g_buffer_render_encoder, &light_proj_view, sizeof(light_proj_view), 3);

                metal_set_fragment_sampler(g_buffer_render_encoder, render_context->shadowmap_sampler, 0);

                metal_set_fragment_texture(g_buffer_render_encoder, render_context->directional_light_shadowmap_depth_texture, 0);

                metal_draw_indexed(g_buffer_render_encoder, MTLPrimitiveTypeTriangle, 
                        render_context->combined_index_buffer.buffer, entry->index_buffer_offset, entry->index_count);
            }break;

            case RenderEntryType_Grass:
            {
                RenderEntryGrass *entry = (RenderEntryGrass *)((u8 *)render_push_buffer->base + consumed);
                consumed += sizeof(*entry);

                m4x4 model = M4x4();
                model = transpose(model); // make the matrix column-major

                PerObjectData per_object_data = {};
                per_object_data.model = model;
                per_object_data.color = entry->color;

                metal_set_render_pipeline(g_buffer_render_encoder, render_context->singlepass_cube_pipeline);

                metal_set_vertex_bytes(g_buffer_render_encoder, &per_object_data, sizeof(per_object_data), 1);
                metal_set_vertex_buffer(g_buffer_render_encoder, 
                                        render_context->combined_vertex_buffer.buffer, 
                                        entry->vertex_buffer_offset, 2);
                metal_set_vertex_bytes(g_buffer_render_encoder, &light_proj_view, sizeof(light_proj_view), 3);

                metal_set_fragment_sampler(g_buffer_render_encoder, render_context->shadowmap_sampler, 0);

                metal_set_fragment_texture(g_buffer_render_encoder, render_context->directional_light_shadowmap_depth_texture, 0);

                metal_draw_indexed(g_buffer_render_encoder, MTLPrimitiveTypeTriangle, 
                        render_context->combined_index_buffer.buffer, entry->index_buffer_offset, entry->index_count);
            }break;

            default:
            {
                consumed += header->size;
            }
        }
    }
    metal_end_encoding(g_buffer_render_encoder);
#endif

    // NOTE(gh) Run deferred lighting on the resulting framebuffer
    id <MTLTexture> drawable_texture =  render_context->view.currentDrawable.texture;
    if(drawable_texture)
    {
        render_context->deferred_renderpass.colorAttachments[0].texture = drawable_texture;
        render_context->deferred_renderpass.colorAttachments[0].clearColor = 
        {
            render_push_buffer->clear_color.r,
            render_push_buffer->clear_color.g,
            render_push_buffer->clear_color.b,
            1.0f
        };

        id<MTLRenderCommandEncoder> deferred_render_encoder = 
            [command_buffer renderCommandEncoderWithDescriptor: render_context->deferred_renderpass];
        deferred_render_encoder.label = @"Deferred Lighting";

        metal_set_viewport(deferred_render_encoder, 0, 0, window_width, window_height, 0, 1);
        metal_set_scissor_rect(deferred_render_encoder, 0, 0, window_width, window_height);
        metal_set_triangle_fill_mode(deferred_render_encoder, MTLTriangleFillModeFill);
        metal_set_front_facing_winding(deferred_render_encoder, MTLWindingCounterClockwise);
        metal_set_cull_mode(deferred_render_encoder, MTLCullModeBack);
        // NOTE(gh) disable depth testing & writing for deferred lighting
        metal_set_depth_stencil_state(deferred_render_encoder, render_context->disabled_depth_state);

        metal_set_render_pipeline(deferred_render_encoder, render_context->deferred_pipeline);

        metal_set_vertex_bytes(deferred_render_encoder, &directional_light_p, sizeof(directional_light_p), 0);
        metal_set_vertex_bytes(deferred_render_encoder, &render_push_buffer->enable_shadow, sizeof(render_push_buffer->enable_shadow), 1);
        metal_set_fragment_texture(deferred_render_encoder, render_context->g_buffer_position_texture, 0);
        metal_set_fragment_texture(deferred_render_encoder, render_context->g_buffer_normal_texture, 1);
        metal_set_fragment_texture(deferred_render_encoder, render_context->g_buffer_color_texture, 2);

        metal_draw_non_indexed(deferred_render_encoder, MTLPrimitiveTypeTriangle, 0, 6);

        metal_end_encoding(deferred_render_encoder);

#if 1
//////// NOTE(gh) Forward rendering start
        render_context->forward_renderpass.colorAttachments[0].texture = drawable_texture;
        id<MTLRenderCommandEncoder> forward_render_encoder = [command_buffer renderCommandEncoderWithDescriptor: render_context->forward_renderpass];
        metal_wait_for_fence(forward_render_encoder, render_context->forward_render_fence, MTLRenderStageFragment);

        metal_set_viewport(forward_render_encoder, 0, 0, window_width, window_height, 0, 1);
        metal_set_scissor_rect(forward_render_encoder, 0, 0, window_width, window_height);
        metal_set_depth_stencil_state(forward_render_encoder, render_context->depth_state);
        metal_set_triangle_fill_mode(forward_render_encoder, MTLTriangleFillModeFill);
        metal_set_front_facing_winding(forward_render_encoder, MTLWindingCounterClockwise);
        metal_set_cull_mode(forward_render_encoder, MTLCullModeBack); 


        // TODO(gh) Also put this inside the game code, and make it as a render entry(with instance count support?)
        if(render_push_buffer->enable_show_perlin_noise_grid)
        {
            for(u32 grass_grid_index = 0;
                    grass_grid_index < render_push_buffer->grass_grid_count_x * render_push_buffer->grass_grid_count_y;
                    ++grass_grid_index)
            {
                GrassGrid *grid = render_push_buffer->grass_grids + grass_grid_index;
                v2 grid_dim = grid->max - grid->min;
                v2 square_dim = V2(grid_dim.x / (f32)grid->grass_count_x, grid_dim.y / (f32)grid->grass_count_y);
                v2u grass_count = V2u(grid->grass_count_x, grid->grass_count_y);

                v2 square_left_bottom_p = grid->min;
                v2 square_right_bottom_p = square_left_bottom_p + V2(square_dim.x, 0);
                v2 square_top_left_p = square_left_bottom_p + V2(0, square_dim.y);
                v2 square_top_right_p = square_top_left_p + V2(square_dim.x, 0);
                v2 square_vertices[] = 
                {
                    square_left_bottom_p,
                    square_right_bottom_p,
                    square_top_left_p,

                    square_right_bottom_p,
                    square_top_right_p,
                    square_top_left_p
                };

                // TODO(gh) Don't think this routine should be this slow, maybe try another instance drawing or 
                // indierct draw method?
                // NOTE(gh) z value will come from the floor z buffer
                metal_set_render_pipeline(forward_render_encoder, render_context->forward_show_perlin_noise_grid_pipeline);

                metal_set_vertex_bytes(forward_render_encoder, square_vertices, array_size(square_vertices), 0);
                metal_set_vertex_bytes(forward_render_encoder, &square_dim, sizeof(square_dim), 1);
                metal_set_vertex_buffer(forward_render_encoder, render_context->giant_buffer.buffer, grid->perlin_noise_buffer_offset, 2);
                metal_set_vertex_buffer(forward_render_encoder, render_context->giant_buffer.buffer, grid->floor_z_buffer_offset, 3);
                metal_set_vertex_bytes(forward_render_encoder, &grass_count, sizeof(grass_count), 4);
                metal_set_vertex_bytes(forward_render_encoder, &per_frame_data, sizeof(per_frame_data), 5);
                metal_draw_non_indexed_instances(forward_render_encoder, MTLPrimitiveTypeTriangle,
                        0, 6, 0, grid->grass_count_x * grid->grass_count_y);
            }
        }

         for(u32 consumed = 0;
                consumed < render_push_buffer->used;
                )
        {
            RenderEntryHeader *header = (RenderEntryHeader *)((u8 *)render_push_buffer->base + consumed);       

            switch(header->type)
            {
                case RenderEntryType_Line :
                {
                    RenderEntryLine *entry = (RenderEntryLine *)((u8 *)render_push_buffer->base + consumed);
                    consumed += header->size;

                    metal_set_render_pipeline(forward_render_encoder, render_context->forward_line_pipeline);
                    metal_set_vertex_bytes(forward_render_encoder, &per_frame_data, sizeof(per_frame_data), 0);

                    v3 vertices[] =
                    {
                        entry->start,
                        entry->end
                    };

                    metal_set_vertex_bytes(forward_render_encoder, vertices, array_size(vertices), 1);
                    metal_set_vertex_bytes(forward_render_encoder, &entry->color, sizeof(entry->color), 2);
                    metal_draw_non_indexed(forward_render_encoder, MTLPrimitiveTypeLine, 0, 2);
                }break;
                case RenderEntryType_Frustum :
                {
                    RenderEntryFrustum *entry = (RenderEntryFrustum *)((u8 *)render_push_buffer->base + consumed);
                    consumed += header->size;

                    metal_set_render_pipeline(forward_render_encoder, render_context->forward_show_game_camera_frustum);
                    metal_set_vertex_buffer(forward_render_encoder, render_context->combined_vertex_buffer.buffer, entry->vertex_buffer_offset, 0);
                    metal_set_vertex_bytes(forward_render_encoder, &render_proj_view, sizeof(render_proj_view), 1);

                    metal_draw_indexed(forward_render_encoder, MTLPrimitiveTypeTriangle, 
                                    render_context->combined_index_buffer.buffer, entry->index_buffer_offset, entry->index_count);

                }break;

                default:
                {
                    consumed += header->size;
                };
            }
        }

#if 0

        v3 screen_space_triangle_vertices[] = 
        {
            {-0.7f, -0.7f, 0.0f},
            {0.7f, -0.7f, 0.0f},
            {-0.7f, 0.7f, 0.0f},

            {0.7f, -0.7f, 0.0f},
            {0.7f, 0.7f, 0.0f},
            {-0.7f, 0.7f, 0.0f},
        };

        metal_set_render_pipeline(forward_render_encoder, render_context->screen_space_triangle_pipeline);
        metal_set_vertex_bytes(forward_render_encoder, screen_space_triangle_vertices, array_size(screen_space_triangle_vertices), 0);
        metal_set_vertex_bytes(forward_render_encoder, &time_elasped_from_start, sizeof(time_elasped_from_start), 1);
        metal_draw_non_indexed(forward_render_encoder, MTLPrimitiveTypeTriangle, 0, array_count(screen_space_triangle_vertices));

#endif

        metal_end_encoding(forward_render_encoder);
#endif

        metal_commit_command_buffer(command_buffer);
         
        // TODO(gh) My understading is metal will automatically sync with the display when I request presenting,
        // but double check
        id<MTLCommandBuffer> present_command_buffer = [render_context->command_queue commandBuffer];

        [present_command_buffer presentDrawable: render_context->view.currentDrawable];
        metal_commit_command_buffer(present_command_buffer);

        // TODO(gh) properly sync with the frame!
    }
}
 

// NOTE(gh): returns the base path where all the folders(code, misc, data) are located
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

internal time_t
macos_get_last_modified_time(char *file_name)
{
    time_t result = 0; 

    struct stat file_stat = {};
    stat(file_name, &file_stat); 
    result = file_stat.st_mtime;

    return result;
}

struct MacOSGameCode
{
    void *library;
    time_t last_modified_time; // u32 bit integer
    UpdateAndRender *update_and_render;
};

internal void
macos_load_game_code(MacOSGameCode *game_code, char *file_name)
{
    // NOTE(gh) dlclose does not actually unload the dll!!!
    // dll only gets unloaded if there is no object that is referencing the dll.
    // TODO(gh) library should be remain open? If so, we need another way to 
    // actually unload the dll so that the fresh dll can be loaded.
    if(game_code->library)
    {
        int error = dlclose(game_code->library);
        game_code->update_and_render = 0;
        game_code->last_modified_time = 0;
        game_code->library = 0;
    }

    void *library = dlopen(file_name, RTLD_LAZY|RTLD_GLOBAL);
    if(library)
    {
        game_code->library = library;
        game_code->last_modified_time = macos_get_last_modified_time(file_name);
        game_code->update_and_render = (UpdateAndRender *)dlsym(library, "update_and_render");
    }
}

int main(void)
{ 
    // srand(time(NULL));
    RandomSeries random_series = start_random_series(rand()); 

    u32 thread_to_spawn_count = 8;
    pthread_attr_t  attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    ThreadWorkQueue thread_work_queue = {};
    thread_work_queue.add_thread_work_queue_item = macos_add_thread_work_item;
    thread_work_queue.complete_all_thread_work_queue_items = macos_complete_all_thread_work_queue_items;

    semaphore = dispatch_semaphore_create(0);

    for(u32 thread_index = 0;
            thread_index < thread_to_spawn_count;
            ++thread_index)
    {
        pthread_t ID = 0;
        MacOSThread *thread = (MacOSThread *)malloc(sizeof(MacOSThread)); 

        thread->ID = thread_index + 1; // 0th thread is the main thread
        thread->queue = &thread_work_queue;

        if(pthread_create(&ID, &attr, &thread_proc, (void *)thread) != 0)
        {
            // TODO(gh) Creating thread failed
            assert(0);
        }
    }
    pthread_attr_destroy(&attr);

    macos_add_thread_work_item(&thread_work_queue, print_string, (void *)"hey! : 0");
    macos_add_thread_work_item(&thread_work_queue, print_string, (void *)"hey! : 1");
    macos_add_thread_work_item(&thread_work_queue, print_string, (void *)"hey! : 2");
    macos_add_thread_work_item(&thread_work_queue, print_string, (void *)"hey! : 3");
    macos_add_thread_work_item(&thread_work_queue, print_string, (void *)"hey! : 4");
    macos_add_thread_work_item(&thread_work_queue, print_string, (void *)"hey! : 5");
    macos_add_thread_work_item(&thread_work_queue, print_string, (void *)"hey! : 6");
    macos_add_thread_work_item(&thread_work_queue, print_string, (void *)"hey! : 7");
    macos_add_thread_work_item(&thread_work_queue, print_string, (void *)"hey! : 8");
    macos_complete_all_thread_work_queue_items(&thread_work_queue);

#if 0
    // Testing font in macos...
    UIFont * arial_font = [fontWithName : [NSString stringWithUTF8String:vertex_shader_name]
                                    size : 40];
#endif

    // TODO(gh) studio display only shows half of the pixels(both width and height)?
    CGDirectDisplayID main_displayID = CGMainDisplayID();
    bool is_display_built_in = CGDisplayIsBuiltin(main_displayID);
    size_t display_width = CGDisplayPixelsWide(main_displayID);
    size_t display_height = CGDisplayPixelsHigh(main_displayID);
    CGSize display_dim = CGDisplayScreenSize(main_displayID);
    u32 display_serial_number = CGDisplaySerialNumber(main_displayID);

    char *lock_file_path = "/Volumes/meka/HB_engine/build/HB.app/Contents/MacOS/lock.tmp";
    char *game_code_path = "/Volumes/meka/HB_engine/build/HB.app/Contents/MacOS/hb.dylib";
    MacOSGameCode macos_game_code = {};
    macos_load_game_code(&macos_game_code, game_code_path);

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

    // TODO(gh) get monitor width and height and use that 
#if 1
    // 2.5k -ish
    i32 window_width = 3200;
    i32 window_height = 1800;
#else
    // 1080p
    i32 window_width = 1920;
    i32 window_height = 1080;
#endif

    u32 target_frames_per_second = 60;
    f32 target_seconds_per_frame = 1.0f/(f32)target_frames_per_second;
    u32 target_nano_seconds_per_frame = (u32)(target_seconds_per_frame*sec_to_nanosec);
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

    // TODO(gh): when connected to the external display, this should be window_width and window_height
    // but if not, this should be window_width/2 and window_height/2. Turns out it's based on the resolution(or maybe ppi),
    // because when connected to the apple studio display, the application should use the same value as the macbook monitor
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
    u64 max_allocation_size = device.recommendedMaxWorkingSetSize;

#if HB_DEBUG
    assert(metal_does_support_gpu_timestamp(device));
#endif
    assert(metal_does_support_gpu_family(device, MTLGPUFamilyApple7)); // Required for mesh shading
    // TODO(gh) MTLGPUFamilyApple8 not defined?
    // assert(metal_does_support_gpu_family(device, MTLGPUFamilyApple8));

    MTKView *view = [[MTKView alloc] initWithFrame : window_rect
                                        device:device];
    CAMetalLayer *metal_layer = (CAMetalLayer*)[view layer];

    // load vkGetInstanceProcAddr
    //macos_initialize_vulkan(&render_context, metal_layer);

    [window setContentView:view];
    view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;

    MetalRenderContext metal_render_context = {};

    metal_render_context.depth_state = metal_make_depth_state(device, MTLCompareFunctionLess, true);
    metal_render_context.disabled_depth_state = metal_make_depth_state(device, MTLCompareFunctionAlways, false);

    NSString *app_path_string = [[NSBundle mainBundle] bundlePath];
    u32 length = [app_path_string lengthOfBytesUsingEncoding: NSUTF8StringEncoding];
    char app_path_without_app_name[512] = {};
    unsafe_string_append(app_path_without_app_name, 
                [app_path_string cStringUsingEncoding: NSUTF8StringEncoding],
                length);

    for(u32 index = length-1;
            index >= 0;
            --index)
    {
        if(app_path_without_app_name[index] == '/')
        {
            break;
        }
        else
        {
            app_path_without_app_name[index] = 0;
        }
    }

    NSError *error;
    // TODO(gh) : Put the metallib file inside the app
    char metallib_path[256] = {};
    unsafe_string_append(metallib_path, base_path);
    unsafe_string_append(metallib_path, "code/shader/shader.metallib");

    // TODO(gh) : maybe just use newDefaultLibrary?
    id<MTLLibrary> shader_library = [device newLibraryWithFile:[NSString stringWithUTF8String:metallib_path] 
                                                                error: &error];
    check_ns_error(error);

    // NOTE(gh) Not entirely sure if this is the right way to do, but I'll just do this
    // because it seems like every pipeline in singlepass should share same color attachments
    MTLPixelFormat singlepass_color_attachment_pixel_formats[] = {MTLPixelFormatRGBA32Float, // position
                                                                  MTLPixelFormatRGBA32Float, // normal
                                                                  MTLPixelFormatRGBA8Unorm}; // color
    MTLColorWriteMask singlepass_color_attachment_write_masks[] = {MTLColorWriteMaskAll, 
                                                                   MTLColorWriteMaskAll,
                                                                   MTLColorWriteMaskAll};
    metal_render_context.singlepass_cube_pipeline = 
        metal_make_render_pipeline(device, "Cube Pipeline", 
                            "singlepass_cube_vertex", "singlepass_cube_frag", 
                            shader_library,
                            MTLPrimitiveTopologyClassTriangle,
                            singlepass_color_attachment_pixel_formats, array_count(singlepass_color_attachment_pixel_formats),
                            singlepass_color_attachment_write_masks, array_count(singlepass_color_attachment_write_masks),
                            view.depthStencilPixelFormat);

    MTLPixelFormat deferred_lighting_pipeline_color_attachment_pixel_formats[] = {MTLPixelFormatBGRA8Unorm};
    MTLColorWriteMask deferred_lighting_pipeline_color_attachment_write_masks[] = {MTLColorWriteMaskAll};
    metal_render_context.deferred_pipeline = 
        metal_make_render_pipeline(device, "Deferred Lighting Pipeline", 
                            "singlepass_deferred_lighting_vertex", "singlepass_deferred_lighting_frag", 
                            shader_library,
                            MTLPrimitiveTopologyClassTriangle,
                            deferred_lighting_pipeline_color_attachment_pixel_formats, array_count(deferred_lighting_pipeline_color_attachment_pixel_formats),
                            deferred_lighting_pipeline_color_attachment_write_masks, array_count(deferred_lighting_pipeline_color_attachment_write_masks),
                            MTLPixelFormatInvalid);

    metal_render_context.directional_light_shadowmap_pipeline = 
        metal_make_render_pipeline(device, "Directional Light Shadowmap Pipeline", 
                            "directional_light_shadowmap_vert", 0, 
                            shader_library,
                            MTLPrimitiveTopologyClassTriangle,
                            0, 0,
                            0, 0,
                            MTLPixelFormatDepth32Float);

    // NOTE(gh) forward pipelines
    // Not every forward pipelines use the blending
    b32 forward_blending_enabled[] = {true};
    b32 forward_blending_disabled[] = {false};

    MTLPixelFormat forward_pipeline_color_attachment_pixel_formats[] = {MTLPixelFormatBGRA8Unorm}; // This is the default pixel format for displaying
    MTLColorWriteMask forward_pipeline_color_attachment_write_masks[] = {MTLColorWriteMaskAll};
    metal_render_context.forward_line_pipeline = 
        metal_make_render_pipeline(device, "Line Pipeline", 
                            "forward_line_vertex", "forward_line_frag",
                            shader_library,
                            MTLPrimitiveTopologyClassLine,
                            forward_pipeline_color_attachment_pixel_formats, array_count(forward_pipeline_color_attachment_pixel_formats),
                            forward_pipeline_color_attachment_write_masks, array_count(forward_pipeline_color_attachment_write_masks),
                            view.depthStencilPixelFormat);

    metal_render_context.screen_space_triangle_pipeline = 
        metal_make_render_pipeline(device, "Sreen Space Triangle Pipeline", "screen_space_triangle_vert", "screen_space_triangle_frag",
                            shader_library,
                            MTLPrimitiveTopologyClassTriangle,
                            forward_pipeline_color_attachment_pixel_formats, array_count(forward_pipeline_color_attachment_pixel_formats),
                            forward_pipeline_color_attachment_write_masks, array_count(forward_pipeline_color_attachment_write_masks),
                            view.depthStencilPixelFormat, forward_blending_disabled);

    metal_render_context.forward_show_perlin_noise_grid_pipeline = 
        metal_make_render_pipeline(device, "Forward Show Perlin Noise Grid Pipeline", 
                            "forward_show_perlin_noise_grid_vert", "forward_show_perlin_noise_grid_frag",
                            shader_library,
                            MTLPrimitiveTopologyClassTriangle,
                            forward_pipeline_color_attachment_pixel_formats, array_count(forward_pipeline_color_attachment_pixel_formats),
                            forward_pipeline_color_attachment_write_masks, array_count(forward_pipeline_color_attachment_write_masks),
                            view.depthStencilPixelFormat, forward_blending_enabled);

    metal_render_context.forward_show_game_camera_frustum = 
        metal_make_render_pipeline(device, "Forward Show Game Camera Frustum Pipeline", 
                            "forward_show_frustum_vert", "forward_show_frustum_frag",
                            shader_library,
                            MTLPrimitiveTopologyClassTriangle,
                            forward_pipeline_color_attachment_pixel_formats, array_count(forward_pipeline_color_attachment_pixel_formats),
                            forward_pipeline_color_attachment_write_masks, array_count(forward_pipeline_color_attachment_write_masks),
                            view.depthStencilPixelFormat, forward_blending_enabled);

    // TODO(gh) These are just temporary numbers, there should be more robust way to get these information
    u32 max_object_thread_count_per_object_threadgroup = 
        object_thread_per_threadgroup_count_x * object_thread_per_threadgroup_count_y; // Each object thread is one grass blade
    u32 max_mesh_threadgroup_count_per_mesh_grid = 
        max_object_thread_count_per_object_threadgroup; // Each mesh thread group is one grass blade
    u32 max_mesh_thread_count_per_mesh_threadgroup = grass_high_lod_index_count;  // TODO(gh) Fix this as multiple of simd width? 
    metal_render_context.grass_mesh_render_pipeline = 
        metal_make_mesh_render_pipeline(device, "Grass Mesh Render Pipeline",
                                        "grass_object_function", 
                                        "single_grass_mesh_function",
                                        "singlepass_cube_frag",
                                        shader_library,
                                        MTLPrimitiveTopologyClassTriangle,
                                        singlepass_color_attachment_pixel_formats, array_count(singlepass_color_attachment_pixel_formats),
                                        singlepass_color_attachment_write_masks, array_count(singlepass_color_attachment_write_masks),
                                        view.depthStencilPixelFormat,
                                        max_object_thread_count_per_object_threadgroup,
                                        max_mesh_threadgroup_count_per_mesh_grid,
                                        max_mesh_thread_count_per_mesh_threadgroup); 

    metal_render_context.initialize_grass_counts_pipeline = 
        metal_make_compute_pipeline(device, shader_library, "initialize_grass_counts");

    metal_render_context.fill_grass_instance_data_pipeline = 
        metal_make_compute_pipeline(device, shader_library, "fill_grass_instance_data_compute");

    metal_render_context.encode_instanced_grass_render_commands_pipeline = 
        metal_make_compute_pipeline(device, shader_library, "encode_instanced_grass_render_commands");

    metal_render_context.instanced_grass_render_pipeline = 
        metal_make_render_pipeline(device, "Instanced Grass Render Pipeline", 
                            "instanced_grass_render_vertex", "singlepass_cube_frag", 
                            shader_library,
                            MTLPrimitiveTopologyClassTriangle,
                            singlepass_color_attachment_pixel_formats, array_count(singlepass_color_attachment_pixel_formats),
                            singlepass_color_attachment_write_masks, array_count(singlepass_color_attachment_write_masks),
                            view.depthStencilPixelFormat, 0, true);

    metal_render_context.grass_start_count_buffer = metal_make_shared_buffer(device, sizeof(u32));
    metal_render_context.grass_count_buffer = metal_make_shared_buffer(device, sizeof(u32));
    // TODO(gh) Should be able to change this based on the game code grass count
    metal_render_context.grass_instance_buffer = metal_make_shared_buffer(device, 32*sizeof(GrassInstanceData)*256*256);

    metal_render_context.grass_index_buffer = metal_make_shared_buffer(device, sizeof(u32)*39);

    u32 grass_high_lod_indices[] = 
    {
        0, 3, 1,
        0, 2, 3,

        2, 5, 3,
        2, 4, 5,

        4, 7, 5,
        4, 6, 7,

        6, 9 ,7,
        6, 8, 9,

        8, 11, 9,
        8, 10, 11,

        10, 13, 11,
        10, 12, 13,

        12, 14, 13,
    };

    for(u32 i = 0;
            i < array_count(grass_high_lod_indices);
            ++i)
    {
        u32 *dst = (u32 *)metal_render_context.grass_index_buffer.memory + i;
        *dst = grass_high_lod_indices[i];
    }

    MTLIndirectCommandBufferDescriptor* icbDescriptor = [MTLIndirectCommandBufferDescriptor new];
    icbDescriptor.commandTypes = MTLIndirectCommandTypeDrawIndexed;
    icbDescriptor.inheritBuffers = false; // inherit 'indirect command buffer' from parent encoder
    icbDescriptor.maxVertexBufferBindCount = 5;
    icbDescriptor.maxFragmentBufferBindCount = 0;
    icbDescriptor.inheritPipelineState = true;

#if 1
    // Create indirect command buffer using private storage mode; since only the GPU will
    // write to and read from the indirect command buffer, the CPU never needs to access the
    // memory
    for(u32 icb_index = 0;
            icb_index < array_count(metal_render_context.icb);
            ++icb_index)
    {
        metal_render_context.icb[icb_index] = 
        [
            device newIndirectCommandBufferWithDescriptor:icbDescriptor
            maxCommandCount:32
            options:MTLResourceStorageModePrivate
        ];

        id<MTLArgumentEncoder> argument_encoder = 
            metal_make_argument_encoder(shader_library, "encode_instanced_grass_render_commands", 0);
        metal_render_context.icb_argument_buffer[icb_index] = metal_make_shared_buffer(device, argument_encoder.encodedLength);

        [argument_encoder setArgumentBuffer:metal_render_context.icb_argument_buffer[icb_index].buffer offset:0]; 

        [argument_encoder setIndirectCommandBuffer:metal_render_context.icb[icb_index]
            atIndex:0];

    }
    #endif

    id<MTLCommandQueue> command_queue = [device newCommandQueue];

    // NOTE(gh) Create required textures
    metal_render_context.g_buffer_position_texture  = 
        metal_make_texture_2D(device, 
                              MTLPixelFormatRGBA32Float, 
                              window_width, 
                              window_height,
                              MTLTextureType2D,
                              MTLTextureUsageRenderTarget,
                              MTLStorageModePrivate);

    metal_render_context.g_buffer_normal_texture  = 
        metal_make_texture_2D(device, 
                              MTLPixelFormatRGBA32Float, 
                              window_width, 
                              window_height,
                              MTLTextureType2D,
                              MTLTextureUsageRenderTarget,
                              MTLStorageModePrivate);

    metal_render_context.g_buffer_color_texture  = 
        metal_make_texture_2D(device, 
                              MTLPixelFormatRGBA8Unorm, 
                              window_width, 
                              window_height,
                              MTLTextureType2D,
                              MTLTextureUsageRenderTarget,
                              MTLStorageModePrivate);

    metal_render_context.g_buffer_depth_texture  = 
        metal_make_texture_2D(device, 
                              MTLPixelFormatDepth32Float, 
                              window_width, 
                              window_height,
                              MTLTextureType2D,
                              MTLTextureUsageRenderTarget,
                              MTLStorageModePrivate);

    // NOTE(gh) Create samplers
    metal_render_context.shadowmap_sampler = 
        metal_make_sampler(device, true, MTLSamplerAddressModeClampToEdge, 
                          MTLSamplerMinMagFilterLinear, MTLSamplerMipFilterNotMipmapped, MTLCompareFunctionLess);

     MTLLoadAction clear_g_buffer_renderpass_color_attachment_load_actions[] = 
        {MTLLoadActionClear,//position
        MTLLoadActionClear,//normal
        MTLLoadActionClear};//color
    MTLStoreAction clear_g_buffer_renderpass_color_attachment_store_actions[] = 
        {MTLStoreActionStore,
        MTLStoreActionStore,
        MTLStoreActionStore};
    id<MTLTexture> clear_g_buffer_renderpass_color_attachment_textures[] = 
        {metal_render_context.g_buffer_position_texture,
        metal_render_context.g_buffer_normal_texture,
        metal_render_context.g_buffer_color_texture};
    v4 clear_g_buffer_renderpass_color_attachment_clear_colors[] = 
        {{0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}};
    metal_render_context.clear_g_buffer_renderpass = 
        metal_make_renderpass(clear_g_buffer_renderpass_color_attachment_load_actions, array_count(clear_g_buffer_renderpass_color_attachment_load_actions),
                              clear_g_buffer_renderpass_color_attachment_store_actions, array_count(clear_g_buffer_renderpass_color_attachment_store_actions),
                              clear_g_buffer_renderpass_color_attachment_textures, array_count(clear_g_buffer_renderpass_color_attachment_textures),
                              clear_g_buffer_renderpass_color_attachment_clear_colors, array_count(clear_g_buffer_renderpass_color_attachment_clear_colors),
                              MTLLoadActionClear, MTLStoreActionStore,  // store depth buffer for the forward pass
                              metal_render_context.g_buffer_depth_texture,
                              1.0f);   

    MTLLoadAction g_buffer_renderpass_color_attachment_load_actions[] = 
        {MTLLoadActionLoad,//position
        MTLLoadActionLoad,//normal
        MTLLoadActionLoad};//color
    MTLStoreAction g_buffer_renderpass_color_attachment_store_actions[] = 
        {MTLStoreActionStore,
        MTLStoreActionStore,
        MTLStoreActionStore};
    id<MTLTexture> g_buffer_renderpass_color_attachment_textures[] = 
        {metal_render_context.g_buffer_position_texture,
        metal_render_context.g_buffer_normal_texture,
        metal_render_context.g_buffer_color_texture};
    v4 g_buffer_renderpass_color_attachment_clear_colors[] = 
        {{0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}};
    metal_render_context.g_buffer_renderpass = 
        metal_make_renderpass(g_buffer_renderpass_color_attachment_load_actions, array_count(g_buffer_renderpass_color_attachment_load_actions),
                              g_buffer_renderpass_color_attachment_store_actions, array_count(g_buffer_renderpass_color_attachment_store_actions),
                              g_buffer_renderpass_color_attachment_textures, array_count(g_buffer_renderpass_color_attachment_textures),
                              g_buffer_renderpass_color_attachment_clear_colors, array_count(g_buffer_renderpass_color_attachment_clear_colors),
                              MTLLoadActionLoad, MTLStoreActionStore,  // store depth buffer for the forward pass
                              metal_render_context.g_buffer_depth_texture,
                              1.0f);

    MTLLoadAction deferred_renderpass_color_attachment_load_actions[] = 
    {
        MTLLoadActionClear,
    };
    MTLStoreAction deferred_renderpass_color_attachment_store_actions[] = 
    {
        MTLStoreActionStore,
    };
    v4 deferred_renderpass_color_attachment_clear_colors[] = 
    {
        {0, 0, 0, 0},
    };
    metal_render_context.deferred_renderpass = metal_make_renderpass
        (
         deferred_renderpass_color_attachment_load_actions, array_count(deferred_renderpass_color_attachment_load_actions),
         deferred_renderpass_color_attachment_store_actions, array_count(deferred_renderpass_color_attachment_store_actions),
         0, 0,
         deferred_renderpass_color_attachment_clear_colors, array_count(deferred_renderpass_color_attachment_clear_colors),
         MTLLoadActionDontCare, MTLStoreActionDontCare,  // store depth buffer for the forward pass
         0,
         1.0f
        );

    MTLLoadAction forward_renderpass_load_actions[] = {MTLLoadActionLoad};
    MTLStoreAction forward_renderpass_store_actions[] = {MTLStoreActionStore};
    metal_render_context.forward_renderpass = 
        metal_make_renderpass(forward_renderpass_load_actions, array_count(forward_renderpass_load_actions),
                              forward_renderpass_store_actions, array_count(forward_renderpass_store_actions),
                              0, 0,
                              0, 0,
                              MTLLoadActionLoad, MTLStoreActionDontCare, 
                              metal_render_context.g_buffer_depth_texture,
                              1.0f);

    // TODO(gh) peter panning happens when the resolution is too low. 
    metal_render_context.directional_light_shadowmap_depth_texture_width = 1024;
    metal_render_context.directional_light_shadowmap_depth_texture_height = 1024;
    metal_render_context.directional_light_shadowmap_depth_texture = 
        metal_make_texture_2D(device, 
                              MTLPixelFormatDepth32Float, 
                              metal_render_context.directional_light_shadowmap_depth_texture_width, 
                              metal_render_context.directional_light_shadowmap_depth_texture_height,
                              MTLTextureType2D,
                              MTLTextureUsageRenderTarget,
                              MTLStorageModePrivate);
    metal_render_context.directional_light_shadowmap_renderpass = 
        metal_make_renderpass(0, 0,
                              0, 0,
                              0, 0, 
                              0, 0,
                              MTLLoadActionClear, MTLStoreActionStore,
                              metal_render_context.directional_light_shadowmap_depth_texture,
                              1.0f);

    metal_render_context.forward_render_fence = metal_make_fence(device);
    for(u32 i = 0;
            i < array_count(metal_render_context.grass_double_buffer_fence);
            ++i)
    {
        metal_render_context.grass_double_buffer_fence[i] = metal_make_fence(device);
    }
    
    metal_render_context.combined_vertex_buffer = metal_make_shared_buffer(device, megabytes(256));
    metal_render_context.combined_index_buffer = metal_make_shared_buffer(device, megabytes(256));

    metal_render_context.giant_buffer = metal_make_shared_buffer(device, gigabytes(1));

    metal_render_context.device = device;
    metal_render_context.view = view;
    metal_render_context.command_queue = command_queue;

    // TODO(gh) More robust way to manage these buffers??(i.e asset system?)

#if 0
    CVDisplayLinkRef display_link;
    if(CVDisplayLinkCreateWithActiveCGDisplays(&display_link) == kCVReturnSuccess)
    {
        CVDisplayLinkSetOutputCallback(display_link, display_link_callback, 0); 
        CVDisplayLinkStart(display_link);

        if(CVDisplayLinkSetCurrentCGDisplay(display_link, main_displayID) == kCVReturnSuccess)
        {
            // TODO(gh) Now we set the display link with the display,
            // the OS will call display link callback function periodically when it wants to display something.
            // Sync this with our display function.
        }
        else
        {
            // TODO(gh) log
            printf("Failed to set the display link with main display\n");
            invalid_code_path;
        }
    }
    else
    {
        // TODO(gh) log
        printf("Failed to create compatible display link with the displays\n");
        invalid_code_path;
    }
#endif

    PlatformInput platform_input = {};

    PlatformRenderPushBuffer platform_render_push_buffer = {};
    platform_render_push_buffer.total_size = megabytes(16);
    platform_render_push_buffer.base = (u8 *)malloc(platform_render_push_buffer.total_size);
    // TODO(gh) Make sure to update this value whenever we resize the window
    platform_render_push_buffer.width_over_height = (f32)window_width / (f32)window_height;

    platform_render_push_buffer.combined_vertex_buffer = metal_render_context.combined_vertex_buffer.memory;
    platform_render_push_buffer.combined_vertex_buffer_size = metal_render_context.combined_vertex_buffer.size;
    platform_render_push_buffer.combined_index_buffer = metal_render_context.combined_index_buffer.memory;
    platform_render_push_buffer.combined_index_buffer_size = metal_render_context.combined_index_buffer.size;
    platform_render_push_buffer.giant_buffer = metal_render_context.giant_buffer.memory;
    platform_render_push_buffer.giant_buffer_size = metal_render_context.giant_buffer.size;

    [app activateIgnoringOtherApps:YES];
    [app run];

    u64 last_time = mach_absolute_time();
    is_game_running = true;
    // TODO(gh) use f64? but metal does not allow double?
    f32 time_elasped_from_start = 0.0f;
    while(is_game_running)
    {
        platform_input.dt_per_frame = target_seconds_per_frame;
        platform_input.time_elasped_from_start = time_elasped_from_start;
        macos_handle_event(app, window, &platform_input);

        // TODO(gh): check if the focued window is working properly
        b32 is_window_focused = [app keyWindow] && [app mainWindow];

        // TODO(gh) : last permission bit should not matter, but double_check?
        int lock_file = open(lock_file_path, O_RDONLY); 
        if(lock_file < 0)
        {
            if(macos_get_last_modified_time(game_code_path) != macos_game_code.last_modified_time)
            {
                // TODO(gh)Do we need to do this?
                macos_complete_all_thread_work_queue_items(&thread_work_queue);
                macos_load_game_code(&macos_game_code, game_code_path);
            }
        }
        else
        {
            close(lock_file);
        }

        if(is_game_running)
        {
            if(macos_game_code.update_and_render)
            {
                macos_game_code.update_and_render(&platform_api, &platform_input, &platform_memory, &platform_render_push_buffer, &thread_work_queue);
            }

            // TODO(gh) Priority queue?

            @autoreleasepool
            {
                metal_render_and_display(&metal_render_context, &platform_render_push_buffer, window_width, window_height, time_elasped_from_start);

                u64 time_passed_in_nsec = clock_gettime_nsec_np(CLOCK_UPTIME_RAW) - last_time;
                u32 time_passed_in_msec = (u32)(time_passed_in_nsec / sec_to_millisec);
                f32 time_passed_in_sec = (f32)time_passed_in_nsec / sec_to_nanosec;
                if(time_passed_in_nsec < target_nano_seconds_per_frame)
                {
                    // NOTE(gh): Because nanosleep is such a high resolution sleep method, for precise timing,
                    // we need to undersleep and spend time in a loop
                    u64 undersleep_nano_seconds = target_nano_seconds_per_frame / 5;
                    if(time_passed_in_nsec + undersleep_nano_seconds < target_nano_seconds_per_frame)
                    {
                        timespec time_spec = {};
                        time_spec.tv_nsec = target_nano_seconds_per_frame - time_passed_in_nsec -  undersleep_nano_seconds;

                        nanosleep(&time_spec, 0);
                    }

                    // For a short period of time, loop
                    time_passed_in_nsec = clock_gettime_nsec_np(CLOCK_UPTIME_RAW) - last_time;
                    while(time_passed_in_nsec < target_nano_seconds_per_frame)
                    {
                        time_passed_in_nsec = clock_gettime_nsec_np(CLOCK_UPTIME_RAW) - last_time;
                    }
                    time_passed_in_msec = (u32)(time_passed_in_nsec / sec_to_millisec);
                    time_passed_in_sec = (f32)time_passed_in_nsec / sec_to_nanosec;
                }
                else
                {
                    // TODO : Missed Frame!
                    // TODO(gh) : Whenever we miss the frame re-sync with the display link
                    // printf("Missed frame, exceeded by %dms(%.6fs)!\n", time_passed_in_msec, time_passed_in_sec);
                }

                time_elasped_from_start += target_seconds_per_frame;
                printf("%dms elasped, fps : %.6f\n", time_passed_in_msec, 1.0f/time_passed_in_sec);
                // printf("CPU:%llu, GPU:%llu\n", metal_render_context.grass_rendering_end_timestamp.cpu - metal_render_context.grass_rendering_start_timestamp.cpu,
                                             // metal_render_context.grass_rendering_end_timestamp.gpu - metal_render_context.grass_rendering_start_timestamp.gpu);
            }

            // update the time stamp
            last_time = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
        }
    }

    return 0;
}











