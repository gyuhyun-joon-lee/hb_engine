#include <Cocoa/Cocoa.h> // APPKIT
#include <CoreGraphics/CoreGraphics.h> 
#include <Metal/metal.h>
#include <metalkit/metalkit.h>
#include <mach/mach_time.h> // mach_absolute_time
#include <stdio.h> // printf for debugging purpose
#include <sys/stat.h>
#include <libkern/OSAtomic.h>
#include <pthread.h>
#include <semaphore.h>
#include <Carbon/Carbon.h>

#include "meka_metal_shader_shared.h"

#undef internal
#undef assert

#include "meka_types.h"
#include "meka_platform.h"
#include "meka_math.h"
#include "meka_random.h"
#include "meka_simd.h"
// NOTE(joon):add platform independent codes here

#include "meka_render.h"
#include "meka_terrain.cpp"
#include "meka_mesh_loader.cpp"
#include "meka_render.cpp"
#include "meka_ray.cpp"
#include "meka_image_loader.cpp"

//TODO(joon) : Put input handlers inside some struct or something
global b32 is_w_down;
global b32 is_a_down;
global b32 is_s_down;
global b32 is_d_down;

global b32 is_i_down;
global b32 is_j_down;
global b32 is_k_down;
global b32 is_l_down;

global b32 is_arrow_up_down;
global b32 is_arrow_down_down;
global b32 is_arrow_left_down;
global b32 is_arrow_right_down;

// TODO(joon): Get rid of global variables?
global v2 last_mouse_p;
global v2 mouse_diff;

global u64 last_time;

global b32 is_game_running;
global dispatch_semaphore_t semaphore;

internal u64 
mach_time_diff_in_nano_seconds(u64 begin, u64 end, r32 nano_seconds_per_tick)
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

    return result;
}

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

    r32 last_frame_time_elapsed =last_frame_diff/(r32)output_time->videoTimeScale;
    r32 current_time_elapsed = current_time_diff/(r32)output_time->videoTimeScale;
    
    //printf("last frame diff: %.6f, current time diff: %.6f\n",  last_frame_time_elapsed, current_time_elapsed);

    last_time = output_time->hostTime;
    return kCVReturnSuccess;
}

internal void
metal_record_render_command(id<MTLRenderCommandEncoder> render_encoder, render_mesh *mesh, 
                            v3 scale, v3 translate, v3 color = v3_(1, 1, 1))
{
    // TODO(joon) : I wonder if Metal copies whatever data it needs, 
    // or does it need a seperate storage that holds all the uniform buffer just like in Vulkan?
    u32 per_object_data_index_in_shader = 2;
    per_object_data per_object_data = {};
    per_object_data.model = translate_m4(translate)*scale_m4(scale);
    per_object_data.color = color;

    [render_encoder setVertexBytes: &per_object_data
                    length: sizeof(per_object_data)
                    atIndex: per_object_data_index_in_shader]; 

    [render_encoder setVertexBuffer: mesh->vertex_buffer
                    offset: 0
                    atIndex:0]; 

    [render_encoder drawIndexedPrimitives: MTLPrimitiveTypeTriangle
                    indexCount: mesh->index_count 
                    indexType: MTLIndexTypeUInt32
                    indexBuffer: mesh->index_buffer 
                    indexBufferOffset: 0 
                    instanceCount: 1]; 
}

internal render_mesh
metal_create_render_mesh_from_raw_mesh(id<MTLDevice> device, raw_mesh *raw_mesh, memory_arena *memory_arena)
{
    assert(raw_mesh->normals);
    assert(raw_mesh->position_count == raw_mesh->normal_count);
    // TODO(joon) : Does not allow certain vertex to have different normal index per face
    if(raw_mesh->normal_indices)
    {
        assert(raw_mesh->indices[0] == raw_mesh->normal_indices[0]);
    }

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

        assert(is_normalized(raw_mesh->normals[vertex_index]));

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

                        else if(key_code == kVK_ANSI_I)
                        {
                            is_i_down = is_down;
                        }
                        else if(key_code == kVK_ANSI_J)
                        {
                            is_j_down = is_down;
                        }
                        else if(key_code == kVK_ANSI_K)
                        {
                            is_k_down = is_down;
                        }
                        else if(key_code == kVK_ANSI_L)
                        {
                            is_l_down = is_down;
                        }

                        else if(key_code == kVK_LeftArrow)
                        {
                            is_arrow_left_down = is_down;
                        }
                        else if(key_code == kVK_RightArrow)
                        {
                            is_arrow_right_down = is_down;
                        }
                        else if(key_code == kVK_UpArrow)
                        {
                            is_arrow_up_down = is_down;
                        }
                        else if(key_code == kVK_DownArrow)
                        {
                            is_arrow_down_down = is_down;
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

struct thread_work_raytrace_tile_data
{
    raytracer_data raytracer_input;
};

global volatile u64 total_bounced_ray_count;
internal 
THREAD_WORK_CALLBACK(thread_work_callback_render_tile)
{
    thread_work_raytrace_tile_data *raytracer_data = (thread_work_raytrace_tile_data *)data;

    raytracer_output output = render_raytraced_image_tile(&raytracer_data->raytracer_input);
    //raytracer_output output = render_raytraced_image_tile_simd(&raytracer_data->raytracer_input);

    // TODO(joon): double check the return value of the OSAtomicIncrement32, is it really a post incremented value? 
    i32 rendered_tile_count = OSAtomicIncrement32Barrier((volatile int32_t *)&raytracer_data->raytracer_input.world->rendered_tile_count);

    u64 ray_count = raytracer_data->raytracer_input.ray_per_pixel_count*
                    (raytracer_data->raytracer_input.one_past_max_x - raytracer_data->raytracer_input.min_x) * 
                    (raytracer_data->raytracer_input.one_past_max_y - raytracer_data->raytracer_input.min_y);

    OSAtomicAdd64Barrier(ray_count, (volatile int64_t *)&raytracer_data->raytracer_input.world->total_ray_count);
    OSAtomicAdd64Barrier(output.bounced_ray_count, (volatile int64_t *)&raytracer_data->raytracer_input.world->bounced_ray_count);

    printf("%dth tile finished with %llu rays\n", rendered_tile_count, raytracer_data->raytracer_input.world->total_ray_count);
}

// TODO(joon) : With current memory arena, we cannot multi-thread the parsing obj
// Make an atomic version(with thread safey) of memory arena?
#if 0
internal
THREAD_WORK_CALLBACK(load_and_parse_obj)
{
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
            dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
        }
    }

    return 0;
}
    
int 
main(void)
{
    simd_f32 temp0 = simd_f32_(12.1f);
    simd_f32 temp1 = simd_f32_(1.0f);

    simd_u32 greater_than_result = compare_greater_equal(temp0, temp1);
    simd_u32 lesser_than_result = compare_less_equal(temp0, temp1);
    
    platform_read_file_result test_png = debug_macos_read_file("/Volumes/meka/meka_renderer/data/test.png");
    platform_read_file_result test_bmp = debug_macos_read_file("/Volumes/meka/meka_renderer/data/test.bmp");
    bmp_file_header *h = (bmp_file_header *)test_bmp.memory;  
    
    load_png(test_png);
    
    struct mach_timebase_info mach_time_info;
    mach_timebase_info(&mach_time_info);
    r32 nano_seconds_per_tick = ((r32)mach_time_info.numer/(r32)mach_time_info.denom);

    u32 output_width = 1920;
    u32 output_height = 1080;

    u32 output_size = sizeof(u32)*output_width*output_height + sizeof(bmp_file_header);
    u8 *output = (u8 *)malloc(output_size);
    zero_memory(output, output_size);
    
    bmp_file_header *bmp_header = (bmp_file_header *)output;  
    bmp_header->file_header = 19778;
    bmp_header->file_size = output_size;
    bmp_header->pixel_offset = sizeof(bmp_file_header);

    bmp_header->header_size = sizeof(bmp_file_header) - 14;
    bmp_header->width = output_width;
    bmp_header->height = output_height;
    bmp_header->color_plane_count = 1;
    bmp_header->bits_per_pixel = 32;
    bmp_header->compression = 3;

    bmp_header->image_size = sizeof(u32)*output_width*output_height;
    bmp_header->pixels_in_meter_x = 11811;
    bmp_header->pixels_in_meter_y = 11811;
    bmp_header->red_mask = 0x00ff0000;
    bmp_header->green_mask = 0x0000ff00;
    bmp_header->blue_mask = 0x000000ff;
    bmp_header->alpha_mask = 0xff000000;

    material materials[6] = {};
    // TODO(joon): objects with emit color seems like working ok only ifbthe emit color is much brighter than the limit.. 
    // are we doing something wrong ?
    materials[0].emit_color = v3_(0.1f, 0.1f, 1.0f);

    materials[1].emit_color = v3_(0.01f, 0.01f, 0.01f);
    materials[1].reflection_color = v3_(0.3f, 0.12f, 0.12f);
    materials[1].reflectivity = 0.0f; 

    materials[2].emit_color = 2*v3_(1.0f, 0.1f, 0.2f);
    materials[2].reflectivity = 0.5f;

    materials[3].reflection_color = v3_(0.3f, 0.7f, 0.2f);
    materials[3].reflectivity = 1.0f;

    materials[4].reflection_color = v3_(0.3f, 0.1f, 0.7f);
    materials[4].reflectivity = 0.6f;

    materials[5].reflection_color = v3_(0.9f, 0.7f, 0.8f);
    materials[5].reflectivity = 0.9f;

    plane planes[1] = {};
    planes[0].normal = v3_(0, 0, 1);
    planes[0].d = 0;
    planes[0].material_index = 1;

    sphere spheres[3] = {};
    spheres[0].center = v3_(0, 0, 0); 
    spheres[0].radius = 1; 
    spheres[0].material_index = 2;

    spheres[1].center = v3_(3, -2, 0.8f); 
    spheres[1].radius = 0.7f; 
    spheres[1].material_index = 3;

    spheres[2].center = v3_(-3, 0, 2); 
    spheres[2].radius = 1; 
    spheres[2].material_index = 4;

    triangle triangles[2] = {};
    triangles[0].v0 = v3_(-30, 7, 0); 
    triangles[0].v1 = v3_(-20, 60, 2); 
    triangles[0].v2 = v3_(0, 4, 2); 
    triangles[0].material_index = 5;

    triangles[1].v0 = v3_(-3, 4, 0.5f); 
    triangles[1].v1 = v3_(3, 4, 0.5f); 
    triangles[1].v2 = v3_(0, 4, 5); 
    triangles[1].material_index = 5;

    world world = {};
    world.material_count = array_count(materials);
    world.materials = materials;

    world.plane_count = array_count(planes);
    world.planes = planes;

    world.sphere_count = array_count(spheres);
    world.spheres = spheres;

    world.triangle_count = array_count(triangles);
    world.triangles = triangles;

    v3 camera_p = v3_(2, -10, 1);

    v3 camera_z_axis = normalize(camera_p); // - V3(0, 0, 0), which is the center of the world
    v3 camera_x_axis = normalize(cross(v3_(0, 0, 1), camera_z_axis));
    v3 camera_y_axis = normalize(cross(camera_z_axis, camera_x_axis));

    r32 film_width = 1.0f;
    r32 film_height = 1.0f;
    if(output_width > output_height)
    {
        film_height *= output_height/(r32)output_width;
    }
    else if(output_width < output_height)
    {
        film_width *= output_width/(r32)output_height;
    }

    r32 half_film_width = film_width/2.0f;
    r32 half_film_height = film_height/2.0f;

    r32 half_x_per_pixel = 0.5f*film_width/output_width;
    r32 half_y_per_pixel = 0.5f*film_height/output_height;
    
    r32 distance_from_camera_to_film = 1.0f;
    v3 film_center = camera_p - distance_from_camera_to_film*camera_z_axis;

    // TODO(joon): m1 pro has 8 high performance cores and 2 efficiency cores,
    // any way to use all of them?
    u32 core_count = 8;

    // NOTE(joon): If the height is a positive value, the bitmap is bottom-up
    u32 *pixels = (u32 *)(output + sizeof(bmp_file_header));
    
    semaphore = dispatch_semaphore_create(0);

    pthread_attr_t thread_attribute = {};
    pthread_attr_init(&thread_attribute);

    thread_work_queue work_queue = {};

    u32 thread_count = 8; // 1;

    // NOTE(joon): spawn threads
    if(thread_count > 1)
    {
        // TODO(joon) : spawn threads based on the core count
        macos_thread *threads = (macos_thread *)malloc(sizeof(macos_thread) * (thread_count - 1));

        for(u32 thread_index = 0;
                thread_index < thread_count;
                ++thread_index)
        {
            macos_thread *thread = threads + thread_index;
            thread->ID = thread_index + 1; // 0th thread is the main thread
            thread->queue = &work_queue;

            pthread_t thread_id; // we don't care about this one, we just generate our own id
            int result = pthread_create(&thread_id, &thread_attribute, &thread_proc, (void *)(threads + thread_index));

            if(result != 0) // 0 is the success value in posix
            {
                assert(0);
            }
        }
    }


    // NOTE(joon): This value should be carefully managed, as it affects to the drainout effect
    // for now, 16 by 16 seems like outputting the best(close to) result(1.57 sec)?
    u32 tile_x_count = 16;
    u32 tile_y_count = 16;
    u32 pixel_count_per_tile_x = output_width/tile_x_count;
    u32 pixel_count_per_tile_y = output_height/tile_y_count;
    u32 ray_per_pixel_count = 1024;

    world.total_tile_count = tile_x_count * tile_y_count;
    world.rendered_tile_count = 0;

    thread_work_raytrace_tile_data *raytracer_datas = (thread_work_raytrace_tile_data *)malloc(sizeof(thread_work_raytrace_tile_data) * tile_x_count * tile_y_count);
    u32 raytracer_data_index = 0;

    u64 begin_raytracing_time = mach_absolute_time();
    for(u32 tile_y = 0;
            tile_y < tile_y_count;
            ++tile_y)
    {
        u32 min_y = tile_y * pixel_count_per_tile_y;
        u32 one_past_max_y = min_y + pixel_count_per_tile_y;

        if(one_past_max_y > output_height)
        {
            one_past_max_y = output_height;
        }

        for(u32 tile_x = 0;
                tile_x < tile_x_count;
                ++tile_x)
        {
            u32 min_x = tile_x * pixel_count_per_tile_x;
            u32 one_past_max_x = min_x + pixel_count_per_tile_x;

            if(one_past_max_x > output_width)
            {
                one_past_max_x = output_width;
            }

            thread_work_raytrace_tile_data *data = raytracer_datas + raytracer_data_index++;
            data->raytracer_input.world = &world;

            data->raytracer_input.pixels = pixels;
            data->raytracer_input.output_width = output_width; 
            data->raytracer_input.output_height = output_height;

            data->raytracer_input.film_center = film_center;  
            data->raytracer_input.film_width = film_width; 
            data->raytracer_input.film_height = film_height;

            data->raytracer_input.camera_p = camera_p;
            data->raytracer_input.camera_x_axis = camera_x_axis;
            data->raytracer_input.camera_y_axis = camera_y_axis; 
            data->raytracer_input.camera_z_axis = camera_z_axis;

            data->raytracer_input.min_x = min_x;
            data->raytracer_input.one_past_max_x = one_past_max_x;
            data->raytracer_input.min_y = min_y;
            data->raytracer_input.one_past_max_y = one_past_max_y;

            data->raytracer_input.ray_per_pixel_count = ray_per_pixel_count;

            // TODO(joon): Get rid of rand(), maybe by using rand instruction set?
            data->raytracer_input.series = start_random_series((u32)rand(), (u32)rand(), (u32)rand(), (u32)rand());

            macos_add_thread_work_item(&work_queue,
                                        thread_work_callback_render_tile,
                                        (void *)data);
        }
    }
    macos_complete_all_thread_work_queue_items(&work_queue);

    // complete all thread work only gurantee that all the works are fired up, 
    // but does not gurantee that all of them are done
    while(world.total_tile_count != world.rendered_tile_count)
    {
    }

    // 7.71052sec
    u64 nano_sec_elapsed = mach_time_diff_in_nano_seconds(begin_raytracing_time, mach_absolute_time(), nano_seconds_per_tick);
    
    printf("Config : %u thread(s), %u lane(s), %u rays/pixel\n", thread_count, MEKA_LANE_WIDTH, ray_per_pixel_count);
    printf("Raytracing finished in : %.6fsec\n", (float)((double)nano_sec_elapsed/(double)sec_to_nano_sec));
    printf("Total Ray Count : %llu, ", world.total_ray_count);
    printf("Bounced Ray Count : %llu, ", world.bounced_ray_count);
    printf("%0.6fns/ray\n", ((double)nano_sec_elapsed/world.bounced_ray_count));

    //usleep(1000000);

    debug_macos_write_entire_file("/Volumes/meka/meka_renderer/data/test.bmp", (void *)output, output_size);

    srand((u32)time(NULL));

	i32 window_width = 1920;
	i32 window_height = 1080;
    r32 window_width_over_height = (r32)window_width/(r32)window_height;

    //TODO : writefile?
    platform_api platform_api = {};
    platform_api.read_file = debug_macos_read_file;
    platform_api.write_entire_file = debug_macos_write_entire_file;
    platform_api.free_file_memory = debug_macos_free_file_memory;

    platform_memory platform_memory = {};

    platform_memory.permanent_memory_size = gigabytes(1);
    platform_memory.transient_memory_size = gigabytes(3);
    u64 total_size = platform_memory.permanent_memory_size + platform_memory.transient_memory_size;
    vm_allocate(mach_task_self(), 
                (vm_address_t *)&platform_memory.permanent_memory,
                total_size, 
                VM_FLAGS_ANYWHERE);

    u32 target_frames_per_second = 120;
    r32 target_seconds_per_frame = 1.0f/(r32)target_frames_per_second;
    u32 target_nano_seconds_per_frame = (u32)(target_seconds_per_frame*sec_to_nano_sec);

    platform_memory.transient_memory = (u8 *)platform_memory.permanent_memory + platform_memory.permanent_memory_size;

    // TODO/joon: clean this memory managing stuffs...
    memory_arena mesh_memory_arena = start_memory_arena(platform_memory.permanent_memory, megabytes(16), &platform_memory.permanent_memory_used);
    memory_arena transient_memory_arena = start_memory_arena(platform_memory.transient_memory, gigabytes(1), &platform_memory.transient_memory_used);

    platform_read_file_result cow_obj_file = platform_api.read_file("/Volumes/meka/meka_renderer/data/cow.obj");
    raw_mesh raw_cow_mesh = parse_obj_tokens(&mesh_memory_arena, cow_obj_file.memory, cow_obj_file.size);
    //generate_vertex_normals(&mesh_memory_arena, &transient_memory_arena, &raw_cow_mesh);
    generate_vertex_normals_simd(&mesh_memory_arena, &transient_memory_arena, &raw_cow_mesh);
    platform_api.free_file_memory(cow_obj_file.memory);

#if 1
    platform_read_file_result suzanne_obj_file = platform_api.read_file("/Volumes/meka/meka_renderer/data/suzanne.obj");
    raw_mesh raw_suzanne_mesh = parse_obj_tokens(&mesh_memory_arena, suzanne_obj_file.memory, suzanne_obj_file.size);

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

    // TODO(joon): when connected to the external display, this should be window_width and window_height
    // but if not, this should be window_width/2 and window_height/2. Why?
    NSRect window_rect = NSMakeRect(100.0f, 100.0f, (r32)window_width, (r32)window_height);

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
       NOTE/Joon : 
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
    id<MTLFunction> frag_phong_lighting = [shader_library newFunctionWithName:@"phong_frag_function"];
    MTLRenderPipelineDescriptor *phong_pipeline_descriptor = [MTLRenderPipelineDescriptor new];
    phong_pipeline_descriptor.label = @"Simple Pipeline";
    phong_pipeline_descriptor.vertexFunction = vertex_function;
    phong_pipeline_descriptor.fragmentFunction = frag_phong_lighting;
    phong_pipeline_descriptor.sampleCount = 1;
    phong_pipeline_descriptor.rasterSampleCount = phong_pipeline_descriptor.sampleCount;
    phong_pipeline_descriptor.rasterizationEnabled = true;
    phong_pipeline_descriptor.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;
    phong_pipeline_descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    phong_pipeline_descriptor.colorAttachments[0].writeMask = MTLColorWriteMaskAll;
    phong_pipeline_descriptor.depthAttachmentPixelFormat = view.depthStencilPixelFormat;

    id<MTLRenderPipelineState> metal_pipeline_state = [device newRenderPipelineStateWithDescriptor:phong_pipeline_descriptor
                                                                 error:&error];

    id<MTLFunction> raytracing_vertex_function = [shader_library newFunctionWithName:@"raytracing_vertex_function"];
    id<MTLFunction> raytracing_frag_function = [shader_library newFunctionWithName:@"raytracing_frag_function"];
    MTLRenderPipelineDescriptor *raytracing_pipeline_descriptor = [MTLRenderPipelineDescriptor new];
    raytracing_pipeline_descriptor.label = @"Raytracing Pipeline";
    raytracing_pipeline_descriptor.vertexFunction = raytracing_vertex_function;
    raytracing_pipeline_descriptor.fragmentFunction = raytracing_frag_function;
    raytracing_pipeline_descriptor.sampleCount = 1;
    raytracing_pipeline_descriptor.rasterSampleCount = raytracing_pipeline_descriptor.sampleCount;
    raytracing_pipeline_descriptor.rasterizationEnabled = true;
    raytracing_pipeline_descriptor.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;
    raytracing_pipeline_descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    raytracing_pipeline_descriptor.colorAttachments[0].writeMask = MTLColorWriteMaskAll;
    raytracing_pipeline_descriptor.depthAttachmentPixelFormat = view.depthStencilPixelFormat;

    id<MTLRenderPipelineState> raytracing_pipline_state = [device newRenderPipelineStateWithDescriptor:raytracing_pipeline_descriptor
                                                                 error:&error];

    id<MTLFunction> line_vertex_function = [shader_library newFunctionWithName:@"line_vertex_function"];
    id<MTLFunction> line_frag_function = [shader_library newFunctionWithName:@"line_frag_function"];
    MTLRenderPipelineDescriptor *line_pipeline_descriptor = [MTLRenderPipelineDescriptor new];
    line_pipeline_descriptor.label = @"Line Pipeline";
    line_pipeline_descriptor.vertexFunction = line_vertex_function;
    line_pipeline_descriptor.fragmentFunction = line_frag_function;
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

    CVDisplayLinkRef display_link;
    if(CVDisplayLinkCreateWithActiveCGDisplays(&display_link)== kCVReturnSuccess)
    {
        CVDisplayLinkSetOutputCallback(display_link, display_link_callback, 0); 
        CVDisplayLinkStart(display_link);
    }

    raw_mesh sphere_raw_mesh = generate_sphere_mesh(100, 100);
    render_mesh sphere_mesh = metal_create_render_mesh_from_raw_mesh(device, &sphere_raw_mesh, &transient_memory_arena);

    m4 bottom_rotation = QuarternionRotationM4(v3_(0, 1, 0), pi_32);
    m4 right_rotation = QuarternionRotationM4(v3_(1, 0, 0), half_pi_32);
    m4 left_rotation = QuarternionRotationM4(v3_(1, 0, 0), -half_pi_32);
    m4 front_rotation = QuarternionRotationM4(v3_(0, 1, 0), half_pi_32);
    m4 back_rotation = QuarternionRotationM4(v3_(0, 1, 0), -half_pi_32);

    memory_arena terrain_memory_arena = start_memory_arena(platform_memory.permanent_memory, megabytes(32), &platform_memory.permanent_memory_used);
    raw_mesh terrains[6] = {};
    render_mesh terrain_meshes[6] = {};
    // top
    terrains[0] = generate_plane_terrain_mesh(&terrain_memory_arena, 100, 100); 
    generate_vertex_normals(&mesh_memory_arena, &transient_memory_arena, terrains + 0);
    terrain_meshes[0] = metal_create_render_mesh_from_raw_mesh(device, terrains + 0, &transient_memory_arena);

    // bottom
    terrains[1] = generate_plane_terrain_mesh(&terrain_memory_arena, 100, 100); 
    for(u32 vertex_index = 0;
            vertex_index < terrains[1].position_count;
            ++vertex_index)
    {
        v3 *position = terrains[1].positions + vertex_index;
        *position = (bottom_rotation*v4_(*position, 1.0f)).xyz;
    }
    generate_vertex_normals(&mesh_memory_arena, &transient_memory_arena, terrains + 1);
    terrain_meshes[1] = metal_create_render_mesh_from_raw_mesh(device, terrains + 1, &transient_memory_arena);

    // left
    terrains[2] = generate_plane_terrain_mesh(&terrain_memory_arena, 100, 100); // left
    for(u32 vertex_index = 0;
            vertex_index < terrains[2].position_count;
            ++vertex_index)
    {
        v3 *position = terrains[2].positions + vertex_index;
        *position = (left_rotation*v4_(*position, 1.0f)).xyz;

        int a = 1;
    }
    generate_vertex_normals(&mesh_memory_arena, &transient_memory_arena, terrains + 2);
    terrain_meshes[2] = metal_create_render_mesh_from_raw_mesh(device, terrains + 2, &transient_memory_arena);

    // right
    terrains[3] = generate_plane_terrain_mesh(&terrain_memory_arena, 100, 100); // right
    for(u32 vertex_index = 0;
            vertex_index < terrains[1].position_count;
            ++vertex_index)
    {
        v3 *position = terrains[3].positions + vertex_index;
        *position = (right_rotation*v4_(*position, 1.0f)).xyz;
    }
    generate_vertex_normals(&mesh_memory_arena, &transient_memory_arena, terrains + 3);
    terrain_meshes[3] = metal_create_render_mesh_from_raw_mesh(device, terrains + 3, &transient_memory_arena);

    // front
    terrains[4] = generate_plane_terrain_mesh(&terrain_memory_arena, 100, 100); // front
    for(u32 vertex_index = 0;
            vertex_index < terrains[1].position_count;
            ++vertex_index)
    {
        v3 *position = terrains[4].positions + vertex_index;
        *position = (front_rotation*v4_(*position, 1.0f)).xyz;
    }
    generate_vertex_normals(&mesh_memory_arena, &transient_memory_arena, terrains + 4);
    terrain_meshes[4] = metal_create_render_mesh_from_raw_mesh(device, terrains + 4, &transient_memory_arena);

    // back
    terrains[5] = generate_plane_terrain_mesh(&terrain_memory_arena, 100, 100); // back
    for(u32 vertex_index = 0;
            vertex_index < terrains[1].position_count;
            ++vertex_index)
    {
        v3 *position = terrains[5].positions + vertex_index;
        *position = (back_rotation*v4_(*position, 1.0f)).xyz;
    }
    generate_vertex_normals(&mesh_memory_arena, &transient_memory_arena, terrains + 5);
    terrain_meshes[5] = metal_create_render_mesh_from_raw_mesh(device, terrains + 5, &transient_memory_arena);

    render_mesh cow_mesh = metal_create_render_mesh_from_raw_mesh(device, &raw_cow_mesh, &transient_memory_arena);
    render_mesh suzanne_mesh = metal_create_render_mesh_from_raw_mesh(device, &raw_suzanne_mesh, &transient_memory_arena);

    u32 test_triangle_count = 64;
    test_triangle *test_triangles = push_array(&mesh_memory_arena, test_triangle, test_triangle_count);

#if 0
    material materials[5] = {};
    materials[0].reflectiveness = 1.0f;
    materials[0].emit_color = V3(0, 0, 0);
    materials[0].reflection_color = V3(random_between_0_1(), random_between_0_1(), random_between_0_1());

    materials[1].reflectiveness = 0.0f;
    materials[1].emit_color = V3(0, 0, 0);
    materials[1].reflection_color = V3(random_between_0_1(), random_between_0_1(), random_between_0_1());

    materials[2].reflectiveness = 0.2f;
    materials[2].emit_color = V3(0, 0, 0);
    materials[2].reflection_color = V3(random_between_0_1(), random_between_0_1(), random_between_0_1());
    
    materials[3].reflectiveness = 0.4f;
    materials[3].emit_color = V3(0, 0, 0);
    materials[3].reflection_color = V3(random_between_0_1(), random_between_0_1(), random_between_0_1());

    materials[4].reflectiveness = 0.0f;
    materials[4].emit_color = V3(1, 0, 0);
    materials[4].reflection_color = V3(0, 0, 0);
#endif

    // TODO(joon): Get rid of rand()
    u32 random_seed = (u32)rand();
    random_series series = start_random_series(random_seed); 

    // NOTE(joon) : creating test triangles for raytracing
    // setVertexBytes can draw up to 85(4kb / 16 / 3) triangles without the index buffer
    for(u32 triangle_index = 0;
            triangle_index < test_triangle_count;
            triangle_index++)
    {
        test_triangle *triangle = test_triangles + triangle_index;

        r32 position_range = 10.0f;
        v3 position = v3_(random_between(&series, -position_range, position_range), 
                        random_between(&series, -position_range, position_range), 
                        random_between(&series, -position_range, position_range));

        triangle->v0 = position + v3_(random_between(&series, 4, 7), random_between(&series, 3, 8), random_between(&series, 3, 8));
        triangle->v1 = position + v3_(random_between(&series, 4, 7), random_between(&series, 3, 8), random_between(&series, 3, 8));
        triangle->v2 = position + v3_(random_between(&series, 4, 7), random_between(&series, 3, 9), random_between(&series, 3, 8));

        triangle->material_index = 0;

    }

    camera camera = {};
    r32 focal_length = 1.0f;
    camera.p = v3_(-10, 0, 0);
    camera.initial_z_axis = v3_(-1, 0, 0);
    camera.initial_x_axis = normalize(cross(v3_(0, 0, 1), camera.initial_z_axis));
    camera.initial_y_axis = normalize(cross(camera.initial_z_axis, camera.initial_x_axis));
     
    r32 light_angle = 0.f;
    r32 light_radius = 1000.0f;

    [app activateIgnoringOtherApps:YES];
    [app run];

    u64 last_time = mach_absolute_time();
    is_game_running = true;
    while(is_game_running)
    {
        macos_handle_event(app, window);

        v3 camera_dir = camera_to_world(-camera.initial_z_axis, camera.initial_x_axis, camera.along_x, 
                                                        camera.initial_y_axis, camera.along_y,
                                                        camera.initial_z_axis, camera.along_z);

        // TODO/joon: check if the focued window is working properly
        b32 is_window_focused = [app keyWindow] && [app mainWindow];

        if(is_window_focused)
        {
            m4 temp_view_matrix = world_to_camera(camera.p, 
                                            camera.initial_x_axis, camera.along_x, 
                                            camera.initial_y_axis, camera.along_y, 
                                            camera.initial_z_axis, camera.along_z);

            m4 proj = projection(focal_length, window_width_over_height, 0.1f, 10000.0f);

            v4 temp = v4_(0, 1, 1, 1);
            v4 view_temp = (temp_view_matrix * temp);
            v4 proj_temp = proj*view_temp;

            r32 camera_speed = 0.1f;

            if(is_w_down)
            {
                camera.p += camera_speed*camera_dir;
            }

            if(is_s_down)
            {
                camera.p -= camera_speed*camera_dir;
            }

            r32 arrow_key_camera_rotation_speed = 0.01f;
            if(is_arrow_up_down || is_i_down)
            {
                camera.along_x += arrow_key_camera_rotation_speed;
            }
            if(is_arrow_down_down || is_k_down)
            {
                camera.along_x -= arrow_key_camera_rotation_speed;
            }
            if(is_arrow_left_down || is_j_down)
            {
                camera.along_y += arrow_key_camera_rotation_speed;
            }
            if(is_arrow_right_down || is_l_down)
            {
                camera.along_y -= arrow_key_camera_rotation_speed;
            }
        }

#if 0
        // NOTE(joon): This weird convention comes from the fact that the camera lookat direction is always (1, 0, 0) in camera space.
        // TODO(joon): Maybe change this to be more understandable value?
        v3 camera_space_left_bottom_corner = V3(focal_length, 0.5f, -0.5f*window_width_over_height);
        r32 film_y = 0.0f;
        for(u32 y = 0;
                y < window_height;
                ++y)
        {
            r32 film_x = 0.0f;
            for(u32 x = 0;
                    x < window_width;
                    ++x)
            {
                v3 result = V3(0, 0, 0);
                u32 ray_count = 8;

                for(u32 ray_index = 0;
                        ray_index < ray_count;
                        ++ray_index)
                {
                    // TODO(joon): Test this code
                    v3 camera_space_pixel_p = camera_space_left_bottom_corner + V3(0, -film_x, film_y);
                    v3 world_space_camera_dir = camera_to_world(camera_space_pixel_p, camera.along_x, camera.along_y, camera.along_z);

                    v3 attenuation = V3(1, 1, 1);

                    v3 ray_origin = camera.p;
                    v3 ray_dir = world_space_camera_dir;
                    for(u32 triangle_index = 0;
                            triangle_index < test_triangle_count;
                            ++triangle_index)
                    {
                        triangle *triangle = test_triangles + triangle_index;
                        v3 *triangle_color = test_triangle_colors + triangle_index;
                        *triangle_color = V3(0, 0, 1);
                        
                        ray_intersect_result ray_intersect_result = ray_intersect_with_triangle(triangle->v0, triangle->v1, triangle->v2, 
                                                                                ray_origin, ray_dir);

                        if(ray_intersect_result.hit)
                        {
                            material *material = materials + triangle_index;
                            result += hadamard(material->emit_color, attenuation);
                            attenuation = hadamard(attenuation, material->reflection_color);

                            ray_origin = ray_intersect_result.next_ray_origin;
                            // TODO(joon) : pure bounce vs random bounce, based on the reflectivity
                            ray_dir = ray_intersect_result.ray_reflection;
                        }
                    }
                }

                film_x += 1.0f;
                result /= ray_count;
            }
            film_y += window_width_over_height;
        }
#endif

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
                [render_encoder setCullMode: MTLCullModeNone];
                //[render_encoder setCullMode: MTLCullModeBack];
                [render_encoder setDepthStencilState: metal_depth_state];

                m4 proj_matrix = projection(focal_length, window_width_over_height, 0.1f, 10000.0f);
                m4 view_matrix = world_to_camera(camera.p, 
                                                 camera.initial_x_axis, camera.along_x, 
                                                 camera.initial_y_axis, camera.along_y, 
                                                 camera.initial_z_axis, camera.along_z);

                m4 proj_view = proj_matrix*camera_rhs_to_lhs(view_matrix);
                per_frame_data per_frame_data = {};
                per_frame_data.proj = convert_m4_to_r32_4x4(proj_matrix);
                per_frame_data.view = convert_m4_to_r32_4x4(camera_rhs_to_lhs(view_matrix));
                v3 light_p = v3_(light_radius*cosf(light_angle), light_radius*sinf(light_angle), 10.f);
                per_frame_data.light_p = convert_to_r32_3(light_p);
                [render_encoder setVertexBytes: &per_frame_data
                                length: sizeof(per_frame_data)
                                atIndex: 1]; 

                // NOTE/joon : make sure you update the non-vertex information first
                per_object_data per_object_data = {};
#if 0
                for(u32 terrain_index = 0;
                        terrain_index < array_count(terrain_meshes);
                        terrain_index++)
                {
                    r32 distance = 4.9f;

                    v3 translate = {};
                    v3 scale = V3(0.1f, 0.1f, 0.1f);

                    switch(terrain_index)
                    {
                        case 0:
                        {
                            // top
                            translate = V3(0, 0, distance);
                        }break;
                        case 1:
                        {
                            // bottom
                            translate = V3(0, 0, -distance);
                        }break;
                        case 2:
                        {
                            // left
                            translate = V3(0, distance, 0);
                        }break;
                        case 3:
                        {
                            // right
                            translate = V3(0, -distance, 0);
                        }break;
                        case 4:
                        {
                            // front
                            translate = V3(distance, 0, 0);
                        }break;
                        case 5:
                        {
                            // back
                            translate = V3(-distance, 0, 0);
                        }break;
                    }

                    metal_record_render_command(render_encoder, terrain_meshes + terrain_index, scale, translate);
                }
#endif
                metal_record_render_command(render_encoder, &cow_mesh, v3_(1, 1, 1), v3_(0, 0, 0));
#if 0
                metal_record_render_command(render_encoder, &suzanne_mesh, V3(20, 20, 20), V3(0, 0, 2));
                metal_record_render_command(render_encoder, &sphere_mesh, V3(3, 3, 3), V3(0, 0, 0));

                [render_encoder setRenderPipelineState:raytracing_pipline_state];

                for(u32 triangle_index = 0;
                        triangle_index < test_triangle_count;
                        ++triangle_index)
                {
                    // NOTE(joon): Draw test triangles one by one, to change each colors
                    per_object_data.color = V3(1, 0, 0);
                    [render_encoder setVertexBytes: &per_object_data
                                    length: sizeof(per_object_data)
                                    atIndex: 2]; 
                    [render_encoder setVertexBytes: test_triangles + triangle_index
                                    length: sizeof(triangle) 
                                    atIndex: 0]; 
                    [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                                    vertexStart:0 
                                    vertexCount:3];
                }
#endif

                [render_encoder setRenderPipelineState:line_pipeline_state];
                //per_object_data.model = convert_m4_to_r32_4x4(translate_m4(V3(0, 0, 0))*scale_m4(10, 10, 1000));
                per_object_data.color = v3_(0, 0.15f, 0.8f);

                [render_encoder setVertexBytes: &per_object_data
                                length: sizeof(per_object_data)
                                atIndex: 2]; 

                line_vertex line_vertices[2];
                line_vertices[0].p = convert_to_r32_3(camera.p + 100.0f*camera_dir);
                line_vertices[1].p = convert_to_r32_3(camera.p + 1000.0f * camera_dir);
                //line_vertices[0].p = {0, 0, 0};
                //line_vertices[1].p = {0, 0, 10000};

                // NOTE(joon): setVertexBytes can only used for data with size up to 4kb
                // with vertex that only has position value(16 bytes), it can draw up to 83 triangles
                // without using the index buffer
                [render_encoder setVertexBytes: line_vertices
                                length: sizeof(line_vertex) * array_count(line_vertices) 
                                atIndex: 0]; 

                [render_encoder drawPrimitives:MTLPrimitiveTypeLine
                                vertexStart:0 
                                vertexCount:2];

                [render_encoder endEncoding];
            
                u64 time_before_presenting = mach_absolute_time();
                u64 time_passed = mach_time_diff_in_nano_seconds(last_time, time_before_presenting, nano_seconds_per_tick);
                u64 time_left_until_present_in_nano_seconds = target_nano_seconds_per_frame - time_passed;
                                                            
                double time_left_until_present_in_sec = time_left_until_present_in_nano_seconds/(double)sec_to_nano_sec;

                // TODO(joon):currentdrawable vs nextdrawable?
                // NOTE(joon): This will work find, as long as we match our display refresh rate with our game
                [command_buffer presentDrawable: view.currentDrawable];
                //[command_buffer presentDrawable:view.currentDrawable
                                //atTime:time_left_until_present_in_sec];
            }

            // NOTE : equivalent to vkQueueSubmit,
            // TODO(joon): Sync with the swap buffer!
            [command_buffer commit];
        }
        light_angle += 0.01f;

        u64 time_passed_in_nano_seconds = mach_time_diff_in_nano_seconds(last_time, mach_absolute_time(), nano_seconds_per_tick);

        // NOTE(joon): Because nanosleep is such a high resolution sleep method, for precise timing,
        // we need to undersleep and spend time in a loop for the precise page flip
        u64 undersleep_nano_seconds = 1000000;
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
    }


    return 0;
}
