/*
 * Written by Gyuhyun Lee
 */

#ifndef HB_PLATFORM_H
#define HB_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hb_types.h" 

#include <math.h>


struct PlatformReadFileResult
{
    u8 *memory;
    u64 size; // TOOD/gh : make this to be at least 64bit
};

#define PLATFORM_GET_FILE_SIZE(name) u64 (name)(const char *filename)
typedef PLATFORM_GET_FILE_SIZE(platform_get_file_size);

#define PLATFORM_READ_FILE(name) PlatformReadFileResult (name)(const char *filename)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_WRITE_ENTIRE_FILE(name) void (name)(const char *file_name, void *memory_to_write, u32 size)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_FREE_FILE_MEMORY(name) void (name)(void *memory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

// TODO(gh) Also make one that releases texture handle
#define PLATFORM_ALLOCATE_AND_ACQUIRE_TEXTURE2D_HANDLE(name) void *(name)(void *device, i32 width, i32 height, i32 bytes_per_pixel)
typedef PLATFORM_ALLOCATE_AND_ACQUIRE_TEXTURE2D_HANDLE(platform_allocate_and_acquire_texture2D_handle);

#define PLATFORM_WRITE_TO_ENTIRE_TEXTURE2D(name) void (name)(void *texture_handle, void *src, i32 width, i32 height, i32 bytes_per_pixel)
typedef PLATFORM_WRITE_TO_ENTIRE_TEXTURE2D(platform_write_to_entire_texture2D);

// TODO(gh) Also make one that releases texture handle
#define PLATFORM_ALLOCATE_AND_ACQUIRE_TEXTURE3D_HANDLE(name) void *(name)(void *device, i32 width, i32 height, i32 depth, i32 bytes_per_pixel)
typedef PLATFORM_ALLOCATE_AND_ACQUIRE_TEXTURE3D_HANDLE(platform_allocate_and_acquire_texture3D_handle);

#define PLATFORM_WRITE_TO_ENTIRE_TEXTURE3D(name) void (name)(void *texture_handle, void *src, i32 width, i32 height, i32 depth, i32 bytes_per_pixel)
typedef PLATFORM_WRITE_TO_ENTIRE_TEXTURE3D(platform_write_to_entire_texture3D);

struct PlatformAPI
{
    platform_read_file *read_file;
    platform_write_entire_file *write_entire_file;
    platform_free_file_memory *free_file_memory;

    // platform_atomic_compare_and_exchange32() *atomic_compare_and_exchange32;
    // platform_atomic_compare_and_exchange64() *atomic_compare_and_exchange64;

    platform_allocate_and_acquire_texture2D_handle *allocate_and_acquire_texture2D_handle;
    platform_write_to_entire_texture2D *write_to_entire_texture2D;

    platform_allocate_and_acquire_texture3D_handle *allocate_and_acquire_texture3D_handle;
    platform_write_to_entire_texture3D *write_to_entire_texture3D;

    // GPU operations
    // TODO(gh) Do we wanna put these here, or do we wanna move into seperate place?
};

struct PlatformInput
{
    b32 move_up;
    b32 move_down;
    b32 move_left;
    b32 move_right;

    b32 action_up;
    b32 action_down;
    b32 action_left;
    b32 action_right;

    b32 space;

    f32 dt_per_frame;
    f32 time_elasped_from_start;
};

// TODO(gh) Make this string to be similar to what Casey has done in his HH project
// which is non-null terminated string with the size
// NOTE(gh) this function has no bound check
internal void
unsafe_string_append(char *dest, const char *source, u32 source_size)
{
    while(*dest != '\0')
    {
        dest++;
    }

    while(source_size-- > 0)
    {
        *dest++ = *source++;
    }
}

// NOTE(gh) this function has no bound check
internal void
unsafe_string_append(char *dest, const char *source)
{
    while(*dest != '\0')
    {
        dest++;
    }
    
    while(*source != '\0')
    {
        *dest++ = *source++;
    }
}

struct PlatformMemory
{
    void *permanent_memory;
    u64 permanent_memory_size;

    void *transient_memory;
    u64 transient_memory_size;
};

// TODO(gh) sub_arena!
struct MemoryArena
{
    void *base;
    size_t total_size;
    size_t used;

    u32 temp_memory_count;
};

internal MemoryArena
start_memory_arena(void *base, size_t size, b32 should_be_zero = true)
{
    MemoryArena result = {};

    result.base = (u8 *)base;
    result.total_size = size;

    // TODO/gh :zeroing memory every time might not be a best idea
    if(should_be_zero)
    {
        zero_memory(result.base, result.total_size);
    }

    return result;
}

// NOTE(gh): Works for both platform memory(world arena) & temp memory
#define push_array(memory, type, count) (type *)push_size(memory, count * sizeof(type))
#define push_struct(memory, type) (type *)push_size(memory, sizeof(type))

// TODO(gh) : Alignment might be an issue, always take account of that
internal void *
push_size(MemoryArena *memory_arena, size_t size, size_t alignment = 0)
{
    assert(size != 0);
    assert(memory_arena->temp_memory_count == 0);
    assert(memory_arena->used <= memory_arena->total_size);

    void *result = (u8 *)memory_arena->base + memory_arena->used;
    memory_arena->used += size;

    return result;
}

struct TempMemory
{
    MemoryArena *memory_arena;

    // TODO/gh: temp memory is for arrays only, so dont need to keep track of 'used'?
    void *base;
    size_t total_size;
    size_t used;
};

// TODO(gh) : Alignment might be an issue, always take account of that
internal void *
push_size(TempMemory *temp_memory, size_t size, size_t alignment = 0)
{
    assert(size != 0);

    void *result = (u8 *)temp_memory->base + temp_memory->used;
    temp_memory->used += size;

    assert(temp_memory->used <= temp_memory->total_size);

    return result;
}

internal TempMemory
start_temp_memory(MemoryArena *memory_arena, size_t size, b32 should_be_zero = true)
{
    TempMemory result = {};
    if(memory_arena)
    {
    result.base = (u8 *)memory_arena->base + memory_arena->used;
    result.total_size = size;
    result.memory_arena = memory_arena;

    push_size(memory_arena, size);

    memory_arena->temp_memory_count++;
    if(should_be_zero)
    {
        zero_memory(result.base, result.total_size);
    }
    }

    return result;
}

// TODO(gh) no need to pass the pointer
internal void
end_temp_memory(TempMemory *temp_memory)
{
    MemoryArena *memory_arena = temp_memory->memory_arena;
    // NOTE(gh) : safe guard for using this temp memory after ending it 
    temp_memory->base = 0;

    memory_arena->temp_memory_count--;
    // IMPORTANT(gh) : As the nature of this, all temp memories should be cleared at once
    memory_arena->used -= temp_memory->total_size;
}

u64 rdtsc(void)
{
	u64 val;
#if HB_ARM 
    // TODO(gh) Counter count seems like it's busted, find another way to do this
	asm volatile("mrs %0, cntvct_el0" : "=r" (val));
#elif HB_X64
    val = __rdtsc();
#endif
	return val;
}

#define PLATFORM_DEBUG_PRINT_CYCLE_COUNTERS(name) void (name)(debug_cycle_counter *debug_cycle_counters)

struct ThreadWorkQueue;
#define THREAD_WORK_CALLBACK(name) void name(void *data)
typedef THREAD_WORK_CALLBACK(ThreadWorkCallback);

// TODO(gh) Good idea?
enum GPUWorkType
{
    GPUWorkType_Null,
    GPUWorkType_AllocateBuffer,
    GPUWorkType_WriteEntireBuffer,
};
struct ThreadAllocateBufferData
{
    void **handle_to_populate;
    u64 size_to_allocate;
};
struct ThreadWriteEntireBufferData
{
    void *handle;

    void *source;
    u64 size_to_write;
};

// TODO(gh) task_with_memory
struct ThreadWorkItem
{
    union
    {
        ThreadWorkCallback *callback; // callback to the function that we wanna execute
        GPUWorkType gpu_work_type;
    };
    void *data;

    b32 written; // indicates whether this item is properly filled or not
};

#define PLATFORM_COMPLETE_ALL_THREAD_WORK_QUEUE_ITEMS(name) void name(ThreadWorkQueue *queue, b32 main_thread_should_do_work)
typedef PLATFORM_COMPLETE_ALL_THREAD_WORK_QUEUE_ITEMS(platform_complete_all_thread_work_queue_items);

#define PLATFORM_ADD_THREAD_WORK_QUEUE_ITEM(name) void name(ThreadWorkQueue *queue, ThreadWorkCallback *thread_work_callback, u32 gpu_work_type, void *data)
typedef PLATFORM_ADD_THREAD_WORK_QUEUE_ITEM(platform_add_thread_work_queue_item);

#define PLATFORM_DO_THREAD_WORK_ITEM(name) b32 (name)(ThreadWorkQueue *queue, u32 thread_index)
typedef PLATFORM_DO_THREAD_WORK_ITEM(platform_do_thread_work_item);

// IMPORTANT(gh): There is no safeguard for the situation where one work takes too long, and meanwhile the work queue was filled so quickly
// causing writeItem == readItem
struct ThreadWorkQueue
{
    void *semaphore;

    // NOTE(gh) : volatile forces the compiler not to optimize the value out, and always to the load(as other thread can change it)
    // NOTE(gh) These two just increments, and the function is responsible for doing the modular 
    int volatile work_index; // index to the queue that is currently under work
    int volatile add_index; // Only the main thread should increment this, as this is not barriered!!!

    int volatile completion_goal;
    int volatile completion_count;

    ThreadWorkItem items[1024];

    // TODO(gh) Not every queue has this!
    void *render_context;

    // now this can be passed onto other codes, such as seperate game code to be used as rendering 
    platform_add_thread_work_queue_item *add_thread_work_queue_item;
    platform_complete_all_thread_work_queue_items * complete_all_thread_work_queue_items;
    // NOTE(gh) Should NOT be used from the game code side!
    platform_do_thread_work_item *_do_thread_work_item;
};

// TODO(gh) Request(game code) - Give(platform layer) system?
struct PlatformRenderPushBuffer
{
    // NOTE(gh) provided by the platform layer
    void *device; // TODO(gh) Not well thought-out concept
    i32 window_width;
    i32 window_height;
    f32 width_over_height; 

    // TODO(gh) Rename this into command buffer or something
    u8 *base;
    u32 total_size;
    u32 used;

    // NOTE(gh) Cleared every frame, includes vertex, index buffer per frame 
    // ONLY USED FOR SMALL MESHES, WHICH THEIR MESH INFORMATION IS NOT KNOWN AT BUILD TIME
    // Good example would be a game frustum(but this can also be replaced with push_quad call)
    void *combined_vertex_buffer;
    u64 combined_vertex_buffer_size;
    u64 combined_vertex_buffer_used;
    void *combined_index_buffer;
    u64 combined_index_buffer_size;
    u64 combined_index_buffer_used;

    // MTLHeap?
    void *giant_buffer;
    u64 giant_buffer_size;
    u64 giant_buffer_used;

//////////// NOTE(gh) game code needs to fill these up
    GrassGrid *grass_grids;
    u32 grass_grid_count_x;
    u32 grass_grid_count_y;

    // This is what the game 'thinks' the camera is
    m4x4 game_camera_view;
    v3 game_camera_p;
    f32 game_camera_near;
    f32 game_camera_far;
    f32 game_camera_fov;

    // This is camera that we are looking into
    m4x4 render_camera_view;
    v3 render_camera_p;
    f32 render_camera_near;
    f32 render_camera_far;
    f32 render_camera_fov;

    v3 clear_color;

    // TODO(gh) Placeholder : Need to think where should we put this info
    v3 sphere_center;
    f32 sphere_r;

    // Configurations
    b32 enable_shadow;
    b32 enable_show_perlin_noise_grid; // show perlin noise on top of the floor
    b32 enable_grass_rendering;
};

#define GAME_UPDATE_AND_RENDER(name) void (name)(PlatformAPI *platform_api, PlatformInput *platform_input, PlatformMemory *platform_memory, PlatformRenderPushBuffer *platform_render_push_buffer, PlatformRenderPushBuffer *debug_platform_render_push_buffer, ThreadWorkQueue *thread_work_queue, ThreadWorkQueue *gpu_work_queue)
typedef GAME_UPDATE_AND_RENDER(UpdateAndRender);

#ifdef __cplusplus
}
#endif

#endif
