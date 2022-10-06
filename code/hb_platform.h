/*
 * Written by Gyuhyun Lee
 */

#ifndef HB_PLATFORM_H
#define HB_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hb_types.h" 

#define assert(expression) if(!(expression)) {int *a = 0; *a = 0;}
#define array_count(array) (sizeof(array) / sizeof(array[0]))
#define array_size(array) (sizeof(array))
#define invalid_code_path assert(0)

#define global static
#define global_variable global
#define local_persist static
#define internal static

#define kilobytes(value) value*1024LL
#define megabytes(value) 1024LL*kilobytes(value)
#define gigabytes(value) 1024LL*megabytes(value)
#define terabytes(value) 1024LL*gigabytes(value)

#define sec_to_nanosec 1.0e+9f
#define sec_to_millisec 1000.0f
//#define nano_sec_to_micro_sec 0.0001f // TODO(gh): Find the correct value :(

#define maximum(a, b) ((a>b)? a:b) 
#define minimum(a, b) ((a<b)? a:b) 

// NOTE(gh): *(u32 *)c == "stri" does not work because of the endianess issues
#define four_cc(string) (((string[0] & 0xff) << 0) | ((string[1] & 0xff) << 8) | ((string[2] & 0xff) << 16) | ((string[3] & 0xff) << 24))

#define tau_32 6.283185307179586476925286766559005768394338798750211641949889f

#define pi_32 3.14159265358979323846264338327950288419716939937510582097494459230f
#define half_pi_32 (pi_32/2.0f)
#define euler_contant 2.7182818284590452353602874713526624977572470936999595749f
#define degree_to_radian(degree) ((degree / 180.0f)*pi_32)

#include <math.h>

struct PlatformReadFileResult
{
    u8 *memory;
    u64 size; // TOOD/gh : make this to be at least 64bit
};

#define PLATFORM_GET_FILE_SIZE(name) u64 (name)(char *filename)
typedef PLATFORM_GET_FILE_SIZE(platform_get_file_size);

#define PLATFORM_READ_FILE(name) PlatformReadFileResult (name)(char *filename)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_WRITE_ENTIRE_FILE(name) void (name)(char *file_name, void *memory_to_write, u32 size)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_FREE_FILE_MEMORY(name) void (name)(void *memory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

struct PlatformAPI
{
    platform_read_file *read_file;
    platform_write_entire_file *write_entire_file;
    platform_free_file_memory *free_file_memory;
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
    f32 time_elapsed_from_start;
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
    result.base = (u8 *)memory_arena->base + memory_arena->used;
    result.total_size = size;
    result.memory_arena = memory_arena;

    push_size(memory_arena, size);

    memory_arena->temp_memory_count++;
    if(should_be_zero)
    {
        zero_memory(result.base, result.total_size);
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

enum debug_cycle_counter_id
{
    debug_cycle_counter_generate_vertex_normals,
    debug_cycle_counter_update_perlin_noise,
    debug_cycle_counter_count
};

struct debug_cycle_counter
{
    u64 cycle_count;
    u32 hit_count;
};

u64 rdtsc(void)
{
	u64 val;
#if HB_ARM 
	asm volatile("mrs %0, cntvct_el0" : "=r" (val));
#elif HB_X64
    val = __rdtsc();
#endif
	return val;
}

global debug_cycle_counter debug_cycle_counters[debug_cycle_counter_count];

#define begin_cycle_counter(ID) u64 begin_cycle_count_##ID = rdtsc();
#define end_cycle_counter(ID) debug_cycle_counters[debug_cycle_counter_##ID].cycle_count += rdtsc() - begin_cycle_count_##ID; \
        debug_cycle_counters[debug_cycle_counter_##ID].hit_count++; \

#define PLATFORM_DEBUG_PRINT_CYCLE_COUNTERS(name) void (name)(debug_cycle_counter *debug_cycle_counters)

struct ThreadWorkQueue;
#define THREAD_WORK_CALLBACK(name) void name(void *data)
typedef THREAD_WORK_CALLBACK(ThreadWorkCallback);

// TODO(gh) task_with_memory
struct ThreadWorkItem
{
    ThreadWorkCallback *callback; // callback to the function that we wanna execute
    void *data;

    b32 written; // indicates whether this item is properly filled or not
};

#define PLATFORM_COMPLETE_ALL_THREAD_WORK_QUEUE_ITEMS(name) void name(ThreadWorkQueue *queue)
typedef PLATFORM_COMPLETE_ALL_THREAD_WORK_QUEUE_ITEMS(platform_complete_all_thread_work_queue_items);

#define PLATFORM_ADD_THREAD_WORK_QUEUE_ITEM(name) void name(ThreadWorkQueue *queue, ThreadWorkCallback *thread_work_callback, void *data)
typedef PLATFORM_ADD_THREAD_WORK_QUEUE_ITEM(platform_add_thread_work_queue_item);

// IMPORTANT(gh): There is no safeguard for the situation where one work takes too long, and meanwhile the work queue was filled so quickly
// causing writeItem == readItem
struct ThreadWorkQueue
{
    // NOTE(gh) : volatile forces the compiler not to optimize the value out, and always to the load(as other thread can change it)
    // NOTE(gh) These two just increments, and the function is responsible for doing the modular 
    int volatile work_index; // index to the queue that is currently under work
    int volatile add_index; // Only the main thread should increment this, as this is not barriered!!!

    ThreadWorkItem items[1024];

    // TODO(gh) Maybe put this in other place
    // now this can be passed onto other codes, such as seperate game code to be used as rendering 
    platform_add_thread_work_queue_item *add_thread_work_queue_item;
    platform_complete_all_thread_work_queue_items * complete_all_thread_work_queue_items;
};

// TODO(gh) Should move these to another file
struct CommonVertex
{
    v3 p;
    v3 normal;
};

// TODO(gh) Only a temporary thing, should move this to the game code?
struct GrassGrid
{
    u32 grass_count_x;
    u32 grass_count_y;

    // NOTE(gh) Platform layer should give enough memory to each of the buffer to 
    // hold grass_count_x*grass_count_y amount of elements. 
    u32 *hash_buffer;
    u32 hash_buffer_size;
    u32 hash_buffer_offset; // offset to the giant buffer, game code should not care about this
    b32 updated_hash_buffer;

    f32 *floor_z_buffer;
    u32 floor_z_buffer_size;
    u32 floor_z_buffer_offset; // offset to the giant buffer, game code should not care about this
    b32 updated_floor_z_buffer;

    // TODO(gh) Also store offset x and y for smooth transition between grids, 
    // or maybe make perlin noise to be seperate from the grid?
    // For now, it is given free because of the fact that every grid is 256 x 256
    f32 *perlin_noise_buffer;
    u32 perlin_noise_buffer_size;
    u32 perlin_noise_buffer_offset; // offset to the giant buffer, game code should not care about this

    v2 min;
    v2 max;
};

// TODO(gh) Request(game code) - Give(platform layer) system?
struct PlatformRenderPushBuffer
{
    // NOTE(gh) provided by the platform layer
    void *device;
    f32 width_over_height; 

    // TODO(gh) Rename this into command buffer or something
    u8 *base;
    u32 total_size;
    u32 used;

    // NOTE(gh) These are cpu side of the buffers that will be updated by the game code, 
    // which need to be updated later to the GPU.
    void *combined_vertex_buffer;
    u32 combined_vertex_buffer_size;
    u32 combined_vertex_buffer_used;

    void *combined_index_buffer;
    u32 combined_index_buffer_size;
    u32 combined_index_buffer_used;

    // MTLHeap?
    void *giant_buffer;
    u64 giant_buffer_size;
    u64 giant_buffer_used;

    GrassGrid grass_grids[4];

    // NOTE(gh) game code needs to fill these up
    m4x4 view;
    v3 camera_p;
    f32 camera_near;
    f32 camera_far;
    f32 camera_fov;
    v3 clear_color;

    // Configurations
    // TODO(gh) Make configuration as struct
    b32 enable_complex_lighting; // when disabled, only use ambient value
    b32 enable_shadow;
    b32 enable_show_perlin_noise_grid; // show perlin noise on top of the floor
    b32 enable_grass_mesh_rendering;
};

#define GAME_UPDATE_AND_RENDER(name) void (name)(PlatformAPI *platform_api, PlatformInput *platform_input, PlatformMemory *platform_memory, PlatformRenderPushBuffer *platform_render_push_buffer, ThreadWorkQueue *thread_work_queue)
typedef GAME_UPDATE_AND_RENDER(UpdateAndRender);

#ifdef __cplusplus
}
#endif

#endif
