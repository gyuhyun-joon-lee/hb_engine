#ifndef MEKA_INTRINSIC_H
#define MEKA_INTRINSIC_H

// NOTE(joon) intrinsic functions are those that the provided by the compiler based on the language(some are non_portable).
// Most of the bit operations are included in this case, including sin, cos functions, too.
// To know more, check https://en.wikipedia.org/wiki/Intrinsic_function

// TODO(joon) Remove this!!!
#include <math.h>

// TODO(joon) add functionality for other compilers(gcc, msvc)
// NOTE(joon) kinda interesting that they do not have compare exchange for floating point numbers
#if MEKA_LLVM

#if MEKA_MACOS
#include <libkern/OSAtomic.h>
// NOTE(joon) : Check https://opensource.apple.com/source/Libc/Libc-594.1.4/i386/sys/OSAtomic.s

// TODO(joon) macos has two different versions of this - with and without 'barrier'.
// Find out whether we acutally need the barrier(for now, we use the version with barrier)
#define atomic_compare_exchange(ptr, expected, desired) OSAtomicCompareAndSwapIntBarrier(ptr, expected, desired)
#define atomic_compare_exchange_64(ptr, expected, desired) OSAtomicCompareAndSwap64Barrier(ptr, expected, desired)

#define atomic_increment(ptr) OSAtomicIncrement32Barrier(ptr)
#define atomic_increment_64(ptr) OSAtomicIncrement64Barrier(ptr)

#define atomic_add(ptr, value_to_add) OSAtomicAdd32Barrier(ptr, value_to_add)
#define atomic_add_64(ptr, value_to_add) OSAtomicAdd64Barrier(ptr, value_to_add)

#elif MEKA_LLVM
// TODO(joon) Can also be used for GCC, because this is a GCC extension of Clang?
//#elif MEKA_GCC

// NOTE(joon) These functions do not care whether it's 32bit or 64bit
#define atomic_compare_exchange(ptr, expected, desired) __sync_bool_compare_and_swap(ptr, expected, desired)
#define atomic_compare_exchange_64(ptr, expected, desired) __sync_bool_compare_and_swap(ptr, expected, desired)

// TODO(joon) Do we really need increment intrinsics?
#define atomic_increment(ptr) __sync_add_and_fetch(ptr, 1)
#define atomic_increment_64(ptr) __sync_add_and_fetch(ptr, 1)

#define atomic_add(ptr, value_to_add) __sync_add_and_fetch(ptr, value_to_add)
#define atomic_add_64(ptr, value_to_add) __sync_add_and_fetch(ptr, value_to_add)
#endif

#elif MEKA_MSVC

#endif

inline u32
count_set_bit(u32 value, u32 size_in_bytes)
{
    u32 result = 0;

    u32 size_in_bit = 8 * size_in_bytes;
    for(u32 bit_shift_index = 0;
            bit_shift_index < size_in_bit;
            ++bit_shift_index)
    {
        if(value & (1 << bit_shift_index))
        {
            result++;
        }
    }

    return result;
}

inline u32
find_most_significant_bit(u8 value)
{
    u32 result = 0;
    for(u32 bit_shift_index = 0;
            bit_shift_index < 8;
            ++bit_shift_index)
    {
        if(value & 128)
        {
            result = bit_shift_index;
            break;
        }

        value = value << 1;
    }

    return result;
}

#define sin(value) sin_(value)
#define cos(value) cos_(value)
#define acos(value) acos_(value)
#define atan2(y, x) atan2_(y, x)

inline r32
sin_(r32 rad)
{
    // TODO(joon) : intrinsic?
    return sinf(rad);
}

inline r32
cos_(r32 rad)
{
    // TODO(joon) : intrinsic?
    return cosf(rad);
}

inline r32
acos_(r32 rad)
{
    return acosf(rad);
}

inline r32
atan2_(r32 y, r32 x)
{
    return atan2f(y, x);
}

inline i32
round_r32_to_i32(r32 value)
{
    // TODO(joon) : intrinsic?
    return (i32)roundf(value);
}

inline u32
round_r32_to_u32(r32 value)
{
    // TODO(joon) : intrinsic?
    return (u32)roundf(value);
}

inline r32
power(r32 base, u32 exponent)
{
    r32 result = powf(base, exponent);
    return result;
}

inline u32
power(u32 base, u32 exponent)
{
    u32 result = 1;
    if(exponent != 0)
    {
        for(u32 i = 0;
                i < exponent;
                ++i)
        {
            result *= base;
        }
    }

    return result; 
}

// TODO(joon) this function can go wrong so easily...
inline u64
pointer_diff(void *start, void *end)
{
    //assert(start && end);
    u64 result = ((u8 *)start - (u8 *)end);

    return result;
}

#include <string.h>
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

#endif
