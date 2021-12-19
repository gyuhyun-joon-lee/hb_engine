/*
 * Written by Gyuhyun 'Joon' Lee
 * https://github.com/meka-lopo/
 */
#ifndef MEKA_PLATFORM_H
#define MEKA_PLATFORM_H

#include <limits.h>
#include <float.h>
#include "meka_types.h" 

typedef int32_t b32;
typedef uintptr_t uintptr;

#define U8_MAX UCHAR_MAX
#define U16_MAX USHRT_MAX
#define U32_MAX UINT_MAX

#define I8_MIN SCHAR_MIN
#define I8_MAX SCHAR_MAX
#define I16_MIN SHRT_MIN
#define I16_MAX SHRT_MAX
#define I32_MIN INT_MIN
#define I32_MAX INT_MAX

typedef float r32;
typedef double r64;

#define assert(Expression) if(!(Expression)) {int *a = 0; *a = 0;}
#define array_count(Array) (sizeof(Array) / sizeof(Array[0]))

#define global static
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

#define pi_32 3.14159265358979323846264338327950288419716939937510582097494459230f
#define half_pi_32 (pi_32/2.0f)

#include "math.h"

inline void
Swap(u16 *a, u16 *b)
{
    u16 temp = *a;
    *a = *b;
    *b = temp;
}

struct platform_read_file_result
{
    u8 *memory;
    u64 size; // TOOD/joon : make this to be at least 64bit
};

#define PLATFORM_GET_FILE_SIZE(name) u64 (name)(char *filename)
typedef PLATFORM_GET_FILE_SIZE(platform_get_file_size);

#define PLATFORM_READ_FILE(name) platform_read_file_result (name)(char *filename)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_WRITE_ENTIRE_FILE(name) void (name)(char *file_name, void *memory_to_write, u32 size)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_FREE_FILE_MEMORY(name) void (name)(void *memory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

struct platform_api
{
    platform_read_file *read_file;
    platform_write_entire_file *write_entire_file;
    platform_free_file_memory *free_file_memory;
};

internal u32
GetCharCountNullTerminated(const char *buffer)
{
    u32 result = 0;
    if(buffer)
    {
        while(buffer[result] != '\0')
        {
            ++result;
        }
    }

    return result;
}

// TODO(joon) : inefficient for long char arrays
// NOTE(joon) : No bound checking for the source char, which can be very dangerous!
internal b32
StringCompare(char *src, char *test)
{
    assert(src && test);

    b32 result = true;
    u32 testCount = GetCharCountNullTerminated(test);

    for(u32 charIndex = 0;
        charIndex < testCount;
        ++charIndex)
    {
        if(src[charIndex] == '\0' ||
            src[charIndex] != test[charIndex])
        {
            result = false;
            break;
        }
    }

    return result;
}

#if MEKA_ARM
#include <arm_neon.h>
#elif MEKA_X64
#include <immintrin.h>
#endif

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

struct platform_memory
{
    void *permanent_memory;
    u64 permanent_memory_size;
    u64 permanent_memory_used;

    void *transient_memory;
    u64 transient_memory_size;
    u64 transient_memory_used;
};

struct memory_arena
{
    void *base;
    size_t total_size;
    size_t used;

    u32 temp_memory_count;
};


internal memory_arena
start_memory_arena(void *base, size_t size, u64 *used, b32 should_be_zero = true)
{
    memory_arena result = {};

    result.base = (u8 *)base + *used;
    result.total_size = size;

    *used += result.total_size;

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
push_size(memory_arena *memory_arena, size_t size, size_t alignment = 0)
{
    assert(size != 0);
    assert(memory_arena->temp_memory_count == 0);
    assert(memory_arena->used < memory_arena->total_size);

    void *result = (u8 *)memory_arena->base + memory_arena->used;
    memory_arena->used += size;

    return result;
}


struct temp_memory
{
    memory_arena *memory_arena;

    // TODO/Joon: temp memory is for arrays only, so dont need to keep track of 'used'?
    void *base;
    size_t total_size;
    size_t used;
};

// TODO(joon) : Alignment might be an issue, always take account of that
internal void *
push_size(temp_memory *temp_memory, size_t size, size_t alignment = 0)
{
    assert(size != 0);

    void *result = (u8 *)temp_memory->base + temp_memory->used;
    temp_memory->used += size;

    assert(temp_memory->used < temp_memory->total_size);

    return result;
}

internal temp_memory
start_temp_memory(memory_arena *memory_arena, size_t size, b32 should_be_zero = true)
{
    temp_memory result = {};
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

internal void
end_temp_memory(temp_memory *temp_memory)
{
    memory_arena *memory_arena = temp_memory->memory_arena;
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

#endif
