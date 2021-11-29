#include <Cocoa/Cocoa.h> // APPKIT
#include <CoreGraphics/CoreGraphics.h> 
#include <Metal/metal.h>
#include <metalkit/metalkit.h>
#include <mach/mach_time.h> // mach_absolute_time
#include <stdio.h> // printf for debugging purpose
#include <sys/stat.h>
#include "meka_metal_shader_shared.h"

#undef internal
#undef assert

#include "meka_platform.h"
// NOTE(joon):add platform independent codes here

#include "meka_render.h"
#include "meka_terrain.cpp"
#include "meka_mesh_loader.cpp"
#include "meka_render.cpp"
PLATFORM_READ_FILE(debug_macos_read_file)
{
    platform_read_file_result Result = {};

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

#if 0
PLATFORM_WRITE_ENTIRE_FILE(debug_macos_write_file)
{
    // NOTE : This call will fail if the file already exists.
    int File = open(fileNameToCreate, O_WRONLY|O_CREAT|O_EXCL, S_IRWXU);
    if(File >= 0) // NOTE : If the open() succeded, the return value is non-negative value.
    {
        if(write(File, memoryToWrite, fileSize) == -1)
        {
            // TODO : LOG here
        }

        close(File);
    }
    else
    {
        // TODO : File already exists. LOG here.
    }
}
#endif
PLATFORM_FREE_FILE_MEMORY(debug_macos_free_file_memory)
{
    free(memory);
}

global_variable b32 is_game_running;

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

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    int a = 1;
}

- (void)windowDidBecomeMain:(NSNotification *)notification
{
    int a = 1;
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

global_variable u64 last_time;

internal CVReturn 
display_link_callback(CVDisplayLinkRef displayLink, const CVTimeStamp* current_time, const CVTimeStamp* output_time,
                CVOptionFlags ignored_0, CVOptionFlags* ignored_1, void* displayLinkContext)
{
    // NOTE(joon) : display link automatically adjust the framerate.
    // TODO(joon) : Find out in what condition it adjusts the framerate?
    u32 last_frame_diff = (u32)(output_time->hostTime - last_time);
    u32 current_time_diff = (u32)(output_time->hostTime - current_time->hostTime);

    r32 last_frame_time_elapsed =last_frame_diff/(r32)output_time->videoTimeScale;
    r32 current_time_elapsed = current_time_diff/(r32)output_time->videoTimeScale;
    
    //printf("last frame diff: %.6f, current time diff: %.6f\n",  last_frame_time_elapsed, current_time_elapsed);

    last_time = output_time->hostTime;
    return kCVReturnSuccess;
}

internal render_mesh
metal_create_render_mesh_from_raw_mesh(id<MTLDevice> device, raw_mesh *raw_mesh, memory_arena *memory_arena)
{
    assert(raw_mesh->normals);
    assert(raw_mesh->position_count == raw_mesh->normal_count);

    // TODO(joon) : not super important, but what should be the size of this
    temp_memory mesh_temp_memory = start_temp_memory(memory_arena, megabytes(16));

    render_mesh result = {};
    temp_vertex *vertices = push_array(&mesh_temp_memory, temp_vertex, raw_mesh->position_count);
    for(u32 vertex_index = 0;
            vertex_index < raw_mesh->position_count;
            ++vertex_index)

    {
        temp_vertex *vertex = vertices + vertex_index;

        // TODO(joon): does vf3 = v3 work just fine?
        vertex->p.x = raw_mesh->positions[vertex_index].x;
        vertex->p.y = raw_mesh->positions[vertex_index].y;
        vertex->p.z = raw_mesh->positions[vertex_index].z;

        v3 src_normal = raw_mesh->normals[vertex_index];
        assert(src_normal.x <=1.0f &&src_normal.y <=1.0f&&src_normal.z <=1.0f);

        vertex->normal.x = raw_mesh->normals[vertex_index].x;
        vertex->normal.y = raw_mesh->normals[vertex_index].y;
        vertex->normal.z = raw_mesh->normals[vertex_index].z;
    }

    // NOTE/joon : MTLResourceStorageModeManaged requires the memory to be explictly synchronized
    u32 vertex_buffer_size = sizeof(temp_vertex)*raw_mesh->position_count;
    result.vertex_buffer = [device newBufferWithBytes: vertices
                                        length: vertex_buffer_size 
                                        options: MTLResourceStorageModeManaged];
    [result.vertex_buffer didModifyRange:NSMakeRange(0, vertex_buffer_size)];

    u32 index_buffer_size = sizeof(raw_mesh->indices[0])*raw_mesh->index_count;
    result.index_buffer = [device newBufferWithBytes: raw_mesh->indices
                                        length: index_buffer_size
                                        options: MTLResourceStorageModeManaged];
    [result.index_buffer didModifyRange:NSMakeRange(0, index_buffer_size)];

    result.index_count = raw_mesh->index_count;

    end_temp_memory(&mesh_temp_memory);

    return result;
}

#include <Carbon/Carbon.h>
//TODO(joon) : obviously not a final code
global_variable b32 is_w_down;
global_variable b32 is_a_down;
global_variable b32 is_s_down;
global_variable b32 is_d_down;

global_variable v2 last_mouse_p;
global_variable v2 mouse_diff;

internal void
macos_handle_event(NSApplication *app, NSWindow *window)
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

    

    r32 mouse_speed_when_clipped = 0.08f;
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

    // NOTE(joon) : Make y to be bottom-up
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
                            is_w_down = is_down;
                        }
                        else if(key_code == kVK_ANSI_A)
                        {
                            is_a_down = is_down;
                        }
                        else if(key_code == kVK_ANSI_S)
                        {
                            is_s_down = is_down;
                        }
                        else if(key_code == kVK_ANSI_D)
                        {
                            is_d_down = is_down;
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

int 
main(void)
{
    srand((u32)time(NULL));


	i32 window_width = 1920;
	i32 window_height = 1080;
    r32 window_width_over_height = (r32)window_width/(r32)window_height;

    //TODO : writefile?
    platform_api platform_api = {};
    platform_api.read_file = debug_macos_read_file;
    platform_api.free_file_memory = debug_macos_free_file_memory;

    platform_memory platform_memory = {};

    platform_memory.permanent_memory_size = gigabytes(1);
    platform_memory.transient_memory_size = gigabytes(3);
    u64 total_size = platform_memory.permanent_memory_size + platform_memory.transient_memory_size;
    vm_allocate(mach_task_self(), 
                (vm_address_t *)&platform_memory.permanent_memory,
                total_size, 
                VM_FLAGS_ANYWHERE);

    platform_memory.transient_memory = (u8 *)platform_memory.permanent_memory + platform_memory.permanent_memory_size;

    // TODO/joon: clean this memory managing stuffs...
    memory_arena mesh_memory_arena = start_memory_arena(platform_memory.permanent_memory, megabytes(16), &platform_memory.permanent_memory_used);
    memory_arena transient_memory_arena = start_memory_arena(platform_memory.transient_memory, gigabytes(1), &platform_memory.transient_memory_used);

    platform_read_file_result cow_obj_file = platform_api.read_file("/Volumes/meka/meka_renderer/data/cow.obj");
    raw_mesh raw_cow_mesh = parse_obj_slow(&mesh_memory_arena, &transient_memory_arena, cow_obj_file.memory, cow_obj_file.size);
    generate_vertex_normals(&mesh_memory_arena, &transient_memory_arena, &raw_cow_mesh);
    platform_api.free_file_memory(cow_obj_file.memory);

#if 1
    platform_read_file_result suzanne_obj_file = platform_api.read_file("/Volumes/meka/meka_renderer/data/suzanne.obj");
    raw_mesh raw_suzanne_mesh = parse_obj_slow(&mesh_memory_arena, &transient_memory_arena, suzanne_obj_file.memory, suzanne_obj_file.size);
    platform_api.free_file_memory(suzanne_obj_file.memory);
#endif

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

    NSRect window_rect = NSMakeRect(100.0f, 100.0f, (r32)window_width/2.0f, (r32)window_height/2.0f);

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

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();

    NSString *name = device.name;
    bool has_unified_memory = device.hasUnifiedMemory;

    MTKView *view = [[MTKView alloc] initWithFrame : window_rect
                                        device:device];
    [window setContentView:view];
    view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;

    /*
       @NOTE/joon : 
       
       METAL combines depth and stencil together.
       To enable depth test inside metal, you need to :
       1. create a metal depth descriptor
       2. create a depth state
       3. pass this whenever we have a command buffer that needs a depth test
    */
    MTLDepthStencilDescriptor *metal_depth_descriptor = [MTLDepthStencilDescriptor new];
    metal_depth_descriptor.depthCompareFunction = MTLCompareFunctionLessEqual;
    metal_depth_descriptor.depthWriteEnabled = true;
    id<MTLDepthStencilState> metal_depth_state = [device newDepthStencilStateWithDescriptor:metal_depth_descriptor];
    [metal_depth_descriptor release];

    //window.contentView = view; // TODO(joon) : use NSViewController to change the view?

    NSError *error;
    // TODO(joon) : Put the metallib file inside the app
    NSString *file_path = @"/Volumes/meka/meka_renderer/code/shader/shader.metallib";
    // TODO(joon) : maybe just use newDefaultLibrary? If so, figure out where should we put the .metal files
    id<MTLLibrary> shader_library = [device newLibraryWithFile:(NSString *)file_path 
                               error: &error];

    check_ns_error(error);

    id<MTLFunction> vertex_function = [shader_library newFunctionWithName:@"vertex_function"];
    id<MTLFunction> frag_phong_lighting = [shader_library newFunctionWithName:@"frag_phong_lighting"];

    MTLRenderPipelineDescriptor *metal_pipeline_descriptor = [MTLRenderPipelineDescriptor new];
    metal_pipeline_descriptor.label = @"Simple Pipeline";
    metal_pipeline_descriptor.vertexFunction = vertex_function;
    metal_pipeline_descriptor.fragmentFunction = frag_phong_lighting;
    metal_pipeline_descriptor.sampleCount = 1;
    metal_pipeline_descriptor.rasterSampleCount = metal_pipeline_descriptor.sampleCount;
    metal_pipeline_descriptor.rasterizationEnabled = true;
    metal_pipeline_descriptor.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;
    metal_pipeline_descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    metal_pipeline_descriptor.colorAttachments[0].writeMask = MTLColorWriteMaskAll;
    metal_pipeline_descriptor.depthAttachmentPixelFormat = view.depthStencilPixelFormat;

    id<MTLRenderPipelineState> metal_pipeline_state = [device newRenderPipelineStateWithDescriptor:metal_pipeline_descriptor
                                                                 error:&error];
    check_ns_error(error);

    id<MTLCommandQueue> command_queue = [device newCommandQueue];

    CVDisplayLinkRef display_link;
    if(CVDisplayLinkCreateWithActiveCGDisplays(&display_link)== kCVReturnSuccess)
    {
        CVDisplayLinkSetOutputCallback(display_link, display_link_callback, 0); 
        CVDisplayLinkStart(display_link);
    }

    memory_arena terrain_memory_arena = start_memory_arena(platform_memory.permanent_memory, megabytes(16), &platform_memory.permanent_memory_used);
    raw_mesh terrain = generate_simple_terrain(&terrain_memory_arena, 100, 100);
    generate_vertex_normals(&mesh_memory_arena, &transient_memory_arena, &terrain);
    render_mesh terrain_mesh = metal_create_render_mesh_from_raw_mesh(device, &terrain, &transient_memory_arena);
    render_mesh cow_mesh = metal_create_render_mesh_from_raw_mesh(device, &raw_cow_mesh, &transient_memory_arena);
#if 1
    render_mesh suzanne_mesh = metal_create_render_mesh_from_raw_mesh(device, &raw_suzanne_mesh, &transient_memory_arena);
#endif

    camera camera = {};
    r32 focal_length = 1.0f;
    camera.p = V3(0, 0, 0);
    
    r32 light_angle = 0.f;
    r32 light_radius = 300.0f;

    [app activateIgnoringOtherApps:YES];
    [app run];

    is_game_running = true;
    while(is_game_running)
    {
        macos_handle_event(app, window);

        // TODO/joon: check if the focued window is working properly
        b32 is_window_focused = [app keyWindow] && [app mainWindow];

        if(is_window_focused)
        {
            // TODO(joon) : more rigorous rotation speed
            camera.alongY += -4.0f*(mouse_diff.x/(r32)window_width);
            camera.alongX += -4.0f*(mouse_diff.y/(r32)window_height);

            r32 camera_speed = 0.05f;
            v3 cameraDir = normalize(QuarternionRotationV001(camera.alongZ, 
                        QuarternionRotationV010(camera.alongY, 
                            QuarternionRotationV100(camera.alongX, V3(0, 0, -1)))));

            // NOTE(joon) : For a rotation matrix, order is x->y->z 
            // which means the matrices should be ordered in z*y*x*vector
            // However, for the view matrix, because the rotation should be inversed,
            // it should be ordered in x*y*z!

            if(is_w_down)
            {
                camera.p += camera_speed*cameraDir;
            }

            if(is_s_down)
            {
                camera.p -= camera_speed*cameraDir;
            }
        }
        /*
            TODO(joon) : For more precisely timed rendering, the operations should be done in this order
            1. Update the game based on the input
            2. Check the mach absolute time
            3. With the return value from the displayLinkOutputCallback function, get the absolute time to present
            4. Use presentDrawable:atTime to present at the specific time
        */
        @autoreleasepool
        {
            id<MTLCommandBuffer> command_buffer = [command_queue commandBuffer];

            MTLViewport viewport = {};
            viewport.originX = 0;
            viewport.originY = 0;
            viewport.width = (r32)window_width;
            viewport.height = (r32)window_height;
            viewport.znear = 0.0;
            viewport.zfar = 1.0;

            // NOTE(joon): renderpass descriptor is already configured for this frame, obtain it as late as possible
            MTLRenderPassDescriptor *this_frame_descriptor = view.currentRenderPassDescriptor;
            //renderpass_descriptor.colorAttachments[0].texture = ;
            this_frame_descriptor.colorAttachments[0].clearColor = {0, 0, 0, 1};
            this_frame_descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            this_frame_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

            this_frame_descriptor.depthAttachment.clearDepth = 1.0f;
            this_frame_descriptor.depthAttachment.loadAction = MTLLoadActionClear;
            this_frame_descriptor.depthAttachment.storeAction = MTLStoreActionDontCare;

            assert(view.currentDrawable);
            CAMetalLayer *layer = (CAMetalLayer*)[view layer];

            if(this_frame_descriptor)
            {
                // NOTE(joon) : encoder is the another layer we need to record commands inside the command buffer.
                // it needs to be created whenever we try to render something 
                id<MTLRenderCommandEncoder> render_encoder = [command_buffer renderCommandEncoderWithDescriptor : this_frame_descriptor];
                render_encoder.label = @"render_encoder";
                [render_encoder setViewport:viewport];
                [render_encoder setRenderPipelineState:metal_pipeline_state];

                [render_encoder setTriangleFillMode: MTLTriangleFillModeFill];
                [render_encoder setFrontFacingWinding: MTLWindingCounterClockwise];
                [render_encoder setCullMode: MTLCullModeBack];
                [render_encoder setDepthStencilState: metal_depth_state];

                vector_float2 shader_viewport = {(r32)window_width, (r32)window_height};
                shader_viewport.x = (r32)window_width;
                shader_viewport.y = (r32)window_height;
                [render_encoder setVertexBytes:&shader_viewport
                                length:sizeof(shader_viewport)
                                atIndex:1];

                m4 proj_matrix = projection(focal_length, window_width_over_height, 0.1f, 1000.0f);
                m4 view_matrix = QuarternionRotationM4(V3(1, 0, 0), -camera.alongX)*
                                QuarternionRotationM4(V3(0, 1, 0), -camera.alongY)*
                                QuarternionRotationM4(V3(0, 0, 1), -camera.alongZ)*
                                Translate(-camera.p.x, -camera.p.y, -camera.p.z);
                m4 proj_view = proj_matrix*view_matrix;
                per_frame_data per_frame_data = {};
                per_frame_data.proj_view = convert_m4_to_r32_4x4(proj_view);
                v3 light_p = V3(light_radius*cosf(light_angle), 10.0f, light_radius*sinf(light_angle));
                per_frame_data.light_p = convert_to_r32_3(light_p);
                [render_encoder setVertexBytes: &per_frame_data
                                length: sizeof(per_frame_data)
                                atIndex: 2]; 

                // NOTE/joon : make sure you update the non-vertex information first
                per_object_data per_object_data = {};
                per_object_data.model = convert_m4_to_r32_4x4(Scale(1, 1, 1));
                [render_encoder setVertexBytes: &per_object_data
                                length: sizeof(per_object_data)
                                atIndex: 3]; 
                [render_encoder setVertexBuffer: terrain_mesh.vertex_buffer
                                offset: 0
                                atIndex:0]; 
                [render_encoder drawIndexedPrimitives: MTLPrimitiveTypeTriangle
                                indexCount: terrain_mesh.index_count 
                                indexType: MTLIndexTypeUInt32
                                indexBuffer: terrain_mesh.index_buffer 
                                indexBufferOffset: 0 
                                instanceCount: 1]; 

                per_object_data.model = convert_m4_to_r32_4x4(Scale(20, 20, 20));
                [render_encoder setVertexBytes: &per_object_data
                                length: sizeof(per_object_data)
                                atIndex: 3]; 
                [render_encoder setVertexBuffer: cow_mesh.vertex_buffer
                                offset: 0
                                atIndex:0]; 
                [render_encoder drawIndexedPrimitives: MTLPrimitiveTypeTriangle
                                indexCount: cow_mesh.index_count 
                                indexType: MTLIndexTypeUInt32
                                indexBuffer: cow_mesh.index_buffer 
                                indexBufferOffset: 0 
                                instanceCount: 1]; 
#if 1
                per_object_data.model = convert_m4_to_r32_4x4(Scale(20, 20, 20));
                [render_encoder setVertexBytes: &per_object_data
                                length: sizeof(per_object_data)
                                atIndex: 3]; 
                [render_encoder setVertexBuffer: suzanne_mesh.vertex_buffer
                                offset: 0
                                atIndex:0]; 
                [render_encoder drawIndexedPrimitives: MTLPrimitiveTypeTriangle
                                indexCount: suzanne_mesh.index_count 
                                indexType: MTLIndexTypeUInt32
                                indexBuffer: suzanne_mesh.index_buffer 
                                indexBufferOffset: 0 
                                instanceCount: 1]; 
#endif


                [render_encoder endEncoding];

                /*
                   TODO(joon): use this method to time with the rendering loop?
                - (void)presentDrawable:(id<MTLDrawable>)drawable 
                         atTime:(CFTimeInterval)presentationTime;
                 */
                // TODO(joon):currentdrawable vs nextdrawable?
                [command_buffer presentDrawable: view.currentDrawable];
            }


            // NOTE : equivalent to vkQueueSubmit
            [command_buffer commit];
        }

        light_angle += 0.00002f;
    }
    return 0;
}
