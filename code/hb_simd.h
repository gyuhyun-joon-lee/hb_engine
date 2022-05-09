#ifndef HB_SIMD_H
#define HB_SIMD_H
// NOTE(joon): This usage of SIMD is a very simple & easy to use to make use of SIMD in a particular peace of code that you want to speed up.
// However, this also creates a bit of an overhead, as everything is wrapped into another layer of function call that might not get inlined or whatever weird thing that
// the compiler might do... So for a routine that needs an _extreme_ speed boot, use this as just a intermission layer, and eventually get rid of it 
// or maybe write an assmebly code

// TODO(joon): Make seperate types for each lane with different size, or a safegurad that we can insert inside the function that tells 
// that a certain function only works with specific size of lane
#define HB_LANE_WIDTH 4 // NOTE(joon): Should be 1/4 for ARM, 1/4/8 for x86/64

#if HB_ARM
//NOTE(joon): Gets rid of bl instructions
#define force_inline static inline __attribute__((always_inline))

#if HB_LANE_WIDTH == 4
#include "hb_simd_4x.h"

#elif HB_LANE_WIDTH == 8

// TODO(joon): M1 pro does not support more that 128bit lane... 
//#include "hb_simd_8x.h"
#elif HB_LANE_WIDTH == 1

typedef v3 simd_v3;
typedef r32 simd_f32;
typedef r32 simd_r32;
typedef u32 simd_u32;
typedef i32 simd_i32;

#define simd_random_series random_series

//////////////////// simd_u32 //////////////////// 
force_inline simd_u32
simd_u32_(u32 value)
{
    return value;
}

force_inline simd_u32
simd_u32_load(u32 *ptr)
{
    simd_u32 result = *ptr;
    return result;
}

force_inline u32
get_lane(simd_u32 a, u32 lane)
{
    return a;
}

force_inline simd_f32
convert_f32_from_u32(simd_u32 a)
{
    // TODO(joon): This might have potential sign bug!!!
    return (simd_f32)a;
}

force_inline simd_u32
compare_not_equal(simd_u32 a, simd_u32 b)
{
    simd_u32 result = 0;
    if(a != b)
    {
        result = 0xffffffff;
    }

    return result;
}

force_inline simd_u32
overwrite(simd_u32 dest, simd_u32 mask, simd_u32 source)
{
    simd_u32 result = {};

    mask = (mask == 0) ? 0 : 0xffffffff;
    result = (dest & (~mask)) | (source & mask);

    return result;
}

force_inline simd_u32
is_lane_non_zero(simd_u32 a)
{
    simd_u32 result = 0;

    if(a != 0)
    {
        result = 0xffffffff;
    }

    return result;
}

force_inline u32
get_non_zero_lane_count_from_all_set_bit(simd_u32 a)
{
    u32 result = 0;

    if(a != 0)
    {
        result = 1;
    }

    return result;
}

force_inline u32
get_non_zero_lane_count(simd_u32 a)
{
    u32 result = 0;

    if(a != 0)
    {
        result = 1;
    }

    return result;
}

force_inline b32
all_lanes_zero(simd_u32 a)
{
    b32 result = false;

    if(a == 0)
    {
        result = true;
    }

    return result;
}

//////////////////// simd_f32 //////////////////// 

force_inline simd_f32
simd_f32_(f32 value)
{
    return value;
}

force_inline simd_f32
simd_f32_(f32 value0, f32 value1, f32 value2, f32 value3)
{
    return value0;
}

force_inline simd_f32
simd_f32_load(f32 *ptr)
{
    simd_f32 result = *ptr;
    return result;
}

force_inline f32
get_lane(simd_f32 a, u32 lane)
{
    return a;
}

force_inline simd_u32
compare_equal(simd_f32 a, simd_f32 b)
{
    simd_u32 result = 0;

    // TODO(joon): This is a totally made up number
    f32 epsilon = 0.00001f;
    f32 diff = a-b;

    if(diff > -epsilon && diff < epsilon)
    {
        result = 0xffffffff;
    }

    return result;
}

force_inline simd_u32
compare_not_equal(simd_f32 a, simd_f32 b)
{
    simd_u32 result = 0;

    result = (compare_equal(a, b) == 0) ? 0xffffffff : 0;

    return result;
}

force_inline simd_u32
compare_greater(simd_f32 a, simd_f32 b)
{
    simd_u32 result = 0;

    if(a > b)
    {
        result = 0xffffffff;
    }

    return result;
}

// TODO(joon): Also add epsilon?
force_inline simd_u32
compare_greater_equal(simd_f32 a, simd_f32 b)
{
    simd_u32 result = 0;

    if(a > b || compare_equal(a, b))
    {
        result = 0xffffffff;
    }

    return result;
}

force_inline simd_u32
compare_less(simd_f32 a, simd_f32 b)
{
    simd_u32 result = 0;
    if(a < b)
    {
        result = 0xffffffff;
    }

    return result;
}

force_inline simd_u32
compare_less_equal(simd_f32 a, simd_f32 b)
{
    simd_u32 result = 0;
    if(a <= b)
    {
        result = 0xffffffff;
    }

    return result;
}

force_inline simd_f32
overwrite(simd_f32 dest, simd_u32 mask, simd_f32 source)
{
    simd_f32 result = {};

    mask = (mask == 0) ? 0 : 0xffffffff;

    // TODO(joon): This is also prone to bug
    simd_u32 result_u32 = (((*(simd_u32 *)&dest) & (~mask)) | ((*(simd_u32 *)&source) & mask));
    result = *(simd_f32 *)&result_u32;

    return result;
}

force_inline simd_f32
add_all_lanes(simd_f32 a)
{
    return a;
}

//////////////////// simd_v3 //////////////////// 

force_inline simd_v3
simd_v3_()
{
    simd_v3 result = {};
    return result;
}

force_inline simd_v3
simd_v3_(v3 value)
{
    return value;
}

force_inline simd_v3
simd_v3_(v3 value0, v3 value1, v3 value2, v3 value3)
{
    simd_v3 result = value0;

    return result;
}

force_inline simd_v3
simd_v3_(simd_f32 value0, simd_f32 value1, simd_f32 value2)
{
    simd_v3 result = {};

    result.x = value0;
    result.y = value1;
    result.z = value2;

    return result;
}

force_inline simd_v3
operator*(simd_v3 a, simd_v3 b)
{
    simd_v3 result = {};

    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;

    return result;
}

force_inline simd_v3
operator/(simd_v3 a, simd_v3 b)
{
    simd_v3 result = {};

    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;

    return result;
}

force_inline simd_v3
min(simd_v3 a, simd_v3 b)
{
    simd_v3 result = {};
    result.x = minimum(a.x, b.x);
    result.y = minimum(a.y, b.y);
    result.z = minimum(a.z, b.z);

    return result;
}

force_inline simd_v3
max(simd_v3 a, simd_v3 b)
{
    simd_v3 result = {};

    result.x = maximum(a.x, b.x);
    result.y = maximum(a.y, b.y);
    result.z = maximum(a.z, b.z);

    return result;
}

force_inline simd_f32
min_component(simd_v3 a)
{
    simd_f32 result = minimum(minimum(a.x, a.y), a.z);

    return result;
}

force_inline simd_f32
max_component(simd_v3 a)
{
    simd_f32 result = maximum(maximum(a.x, a.y), a.z);

    return result;
}

force_inline simd_v3
overwrite(simd_v3 dest, simd_u32 mask, simd_v3 source)
{
    simd_v3 result = {};

    mask = (mask == 0) ? 0 : 0xffffffff;

    result.x = overwrite(dest.x, mask, source.x);
    result.y = overwrite(dest.y, mask, source.y);
    result.z = overwrite(dest.z, mask, source.z);

    return result;
}

force_inline simd_random_series
start_random_series(u32 seed0, u32 seed1, u32 seed2, u32 seed3)
{
    simd_random_series result = {};
    result.next_random = simd_u32_(seed0);

    return result;
}

#endif

///////////// codes that are common across different simd lane size should come here


// TODO(joon) : lane vs non-lane ARM SIMD
// TODO(joon) : load simd aligned vs unaligned
#define dup_u32_128(value)  vdupq_n_u32(value)
#define load_u32_128(ptr) vld1q_u32(ptr)
#define load_r32_128(ptr) vld1q_f32(ptr)
#define sqrt_r32_128(v) vsqrtq_f32(v)
#define conv_u32_to_r32_128(v) vcvtq_f32_u32(v) // NOTE(joon): convert actually re order the bit pattern(similar to c style cast), while re-interpret does not.

#define sub_r32_128(a, b) vsubq_f32(a, b)
#define mul_r32_128(a, b) vmulq_f32(a, b)
#define add_r32_128(a, b) vaddq_f32(a, b)
#define div_r32_128(a, b) vdivq_f32(a, b)

#define u32_from_128(vector, slot) (((u32 *)&vector)[slot])
#define r32_from_128(vector, slot) (((r32 *)&vector)[slot])

#elif HB_X64
#define dup_u32_128(value) _mm_set1_ps(value) 
// NOTE(joon): no unsigned value for _m128i
#define load_u32_128(ptr) _mm_load_si128(ptr)
#define load_i32_128(ptr) _mm_load_si128(ptr)
#define load_r32_128(ptr) _mm_load_ps(ptr)

#define sub_r32_128(a, b) _mm_sub_ps(a, b)
#define sub_u32_128(a, b) _mm_sub_epi32(a, b)
#define mul_r32_128(a, b) _mm_mul_ps(a, b)
#define add_r32_128(a, b) _mm_add_ps(a, b)
#define div_r32_128(a, b) _mm_div_ps(a, b)
#define sqrt_r32_128(v) _mm_sqrt_ps(v)
#define conv_u32_to_r32_128(v) _mm_cvtepi32_ps(v)

 
#endif

#endif
