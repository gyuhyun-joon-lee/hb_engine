/*
 * Written by Gyuhyun 'Joon' Lee
 * https://github.com/meka-lopo/
 */

#ifndef MEKA_PLATFORM_H
#define MEKA_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "meka_types.h" 

#define assert(expression) if(!(expression)) {int *a = 0; *a = 0;}
#define array_count(array) (sizeof(array) / sizeof(array[0]))
#define invalid_code_path assert(0)

#define global static
#define global_variable global
#define local_persist static
#define internal static

#define kilobytes(value) value*1024LL
#define megabytes(value) 1024LL*kilobytes(value)
#define gigabytes(value) 1024LL*megabytes(value)
#define terabytes(value) 1024LL*gigabytes(value)

#define sec_to_nano_sec 1.0e+9f
#define sec_to_micro_sec 1000.0f
//#define nano_sec_to_micro_sec 0.0001f // TODO(joon): Find the correct value :(

#define maximum(a, b) ((a>b)? a:b) 
#define minimum(a, b) ((a<b)? a:b) 

// NOTE(joon): *(u32 *)c == "stri" does not work because of the endianess issues
#define four_cc(string) (((string[0] & 0xff) << 0) | ((string[1] & 0xff) << 8) | ((string[2] & 0xff) << 16) | ((string[3] & 0xff) << 24))

#define pi_32 3.14159265358979323846264338327950288419716939937510582097494459230f
#define half_pi_32 (pi_32/2.0f)

#include "math.h"

struct PlatformReadFileResult
{
    u8 *memory;
    u64 size; // TOOD/joon : make this to be at least 64bit
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
};

// TODO/Joon: intrinsic zero memory?
// TODO(joon): can be faster using wider vectors
inline void
zero_memory(void *memory, u64 size)
{
    // TODO/joon: What if there's no neon support
#if MEKA_ARM
    u8 *byte = (u8 *)memory;
    uint8x16_t zero_128 = vdupq_n_u8(0);

    while(size > 16)
    {
        vst1q_u8(byte, zero_128);

        byte += 16;
        size -= 16;
    }

    if(size > 0)
    {
        while(size--)
        {
            *byte++ = 0;
        }
    }
#else
    // TODO(joon): support for intel simd, too!
    memset (memory, 0, size);
#endif
}

// TODO(joon): Intrinsic?
inline u8
reverse_bits(u8 value)
{
    u8 result = 0;

    for(u32 i = 0;
            i < 8;
            ++i)
    {
        if(((value >> i) & 1) == 0)
        {
            result |= (1 << i);
        }
    }

    return result;
}

struct PlatformMemory
{
    void *permanent_memory;
    u64 permanent_memory_size;

    void *transient_memory;
    u64 transient_memory_size;
};

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

    // TODO/joon :zeroing memory every time might not be a best idea
    if(should_be_zero)
    {
        zero_memory(result.base, result.total_size);
    }

    return result;
}

// NOTE(joon): Works for both platform memory(world arena) & temp memory
#define push_array(memory, type, count) (type *)push_size(memory, count * sizeof(type))
#define push_struct(memory, type) (type *)push_size(memory, sizeof(type))

// TODO(joon) : Alignment might be an issue, always take account of that
internal void *
push_size(MemoryArena *memory_arena, size_t size, size_t alignment = 0)
{
    assert(size != 0);
    assert(memory_arena->temp_memory_count == 0);
    assert(memory_arena->used < memory_arena->total_size);

    void *result = (u8 *)memory_arena->base + memory_arena->used;
    memory_arena->used += size;

    return result;
}

struct TempMemory
{
    MemoryArena *memory_arena;

    // TODO/Joon: temp memory is for arrays only, so dont need to keep track of 'used'?
    void *base;
    size_t total_size;
    size_t used;
};

// TODO(joon) : Alignment might be an issue, always take account of that
internal void *
push_size(TempMemory *temp_memory, size_t size, size_t alignment = 0)
{
    assert(size != 0);

    void *result = (u8 *)temp_memory->base + temp_memory->used;
    temp_memory->used += size;

    assert(temp_memory->used < temp_memory->total_size);

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

// TODO(joon) no need to pass the pointer
internal void
end_temp_memory(TempMemory *temp_memory)
{
    MemoryArena *memory_arena = temp_memory->memory_arena;
    // NOTE(joon) : safe guard for using this temp memory after ending it 
    temp_memory->base = 0;

    memory_arena->temp_memory_count--;
    // IMPORTANT(joon) : As the nature of this, all temp memories should be cleared at once
    memory_arena->used -= temp_memory->total_size;
}

enum debug_cycle_counter_id
{
    debug_cycle_counter_generate_vertex_normals,
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
#if MEKA_ARM 
	asm volatile("mrs %0, cntvct_el0" : "=r" (val));
#elif MEKA_X64
    val = __rdtsc();
#endif
	return val;
}

global debug_cycle_counter debug_cycle_counters[debug_cycle_counter_count];

#define begin_cycle_counter(ID) u64 begin_cycle_count_##ID = rdtsc();
#define end_cycle_counter(ID) debug_cycle_counters[debug_cycle_counter_##ID].cycle_count += rdtsc() - begin_cycle_count_##ID; \
        debug_cycle_counters[debug_cycle_counter_##ID].hit_count++; \

#define PLATFORM_DEBUG_PRINT_CYCLE_COUNTERS(name) void (name)(debug_cycle_counter *debug_cycle_counters)

struct thread_work_queue;
#define THREAD_WORK_CALLBACK(name) void name(void *data)
typedef THREAD_WORK_CALLBACK(thread_work_callback);

struct thread_work_item
{
    thread_work_callback *callback;
    void *data;

    b32 written; // indicates whether this item is properly filled or not
};

#define PLATFORM_COMPLETE_ALL_THREAD_WORK_QUEUE_ITEMS(name) void name(thread_work_queue *queue)
typedef PLATFORM_COMPLETE_ALL_THREAD_WORK_QUEUE_ITEMS(platform_complete_all_thread_work_queue_items);

#define PLATFORM_ADD_THREAD_WORK_QUEUE_ITEM(name) void name(thread_work_queue *queue, thread_work_callback *threadWorkCallback, void *data)
typedef PLATFORM_ADD_THREAD_WORK_QUEUE_ITEM(platform_add_thread_work_queue_item);

// IMPORTANT(joon): There is no safeguard for the situation where one work takes too long, and meanwhile the work queue was filled so quickly
// causing writeItem == readItem
struct thread_work_queue
{
    // NOTE(joon) : volatile forces the compiler not to optimize the value out, and always to the load(as other thread can change it)
    int volatile work_index; // index to the queue that is currently under work
    int volatile add_index;

    thread_work_item items[1024];

    // now this can be passed onto other codes, such as seperate game code to be used as rendering 
    platform_add_thread_work_queue_item *add_thread_work_queue_item;
    platform_complete_all_thread_work_queue_items * complete_all_thread_work_queue_items;
};

#ifdef __cplusplus
}
#endif

#endif
