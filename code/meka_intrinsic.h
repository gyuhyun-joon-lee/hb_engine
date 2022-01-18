#ifndef MEKA_INTRINSIC_H
#define MEKA_INTRINSIC_H

// TODO(joon) intrinsic!
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
round_r32_i32(r32 value)
{
    // TODO(joon) : intrinsic?
    return (i32)roundf(value);
}

inline u32
round_r32_u32(r32 value)
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
    assert(start && end);
    u64 result = ((u8 *)start - (u8 *)end);

    return result;
}

#endif
