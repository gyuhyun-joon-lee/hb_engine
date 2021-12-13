#ifndef MEKA_SIMD_H
#define MEKA_SIMD_H
// TODO(joon): A huge problem with using SIMD like this is that not every commands are matched 1:1
// for example, there is no ARM intrinsic function that has the same functionality with intel's _mm_movemask_epi8 
// which might require us a whole different approach for certain cases

#if MEKA_ARM
// TODO(joon) : lane vs non-lane ARM SIMD
// TODO(joon) : load simd aligned vs unaligned
#define dup_u32_128(value)  vdupq_n_u32(value)
#define load_u32_128(ptr) vld1q_u32(ptr)
#define load_r32_128(ptr) vld1q_f32(ptr)
#define sqrt_r32_128(v) vsqrtq_f32(v)
#define conv_u32_to_r32_128(v) vcvtq_f32_u32(v)

#define sub_r32_128(a, b) vsubq_f32(a, b)
#define mul_r32_128(a, b) vmulq_f32(a, b)
#define add_r32_128(a, b) vaddq_f32(a, b)
#define div_r32_128(a, b) vdivq_f32(a, b)

#define u32_from_128(vector, slot) (((u32 *)&vector)[slot])
#define r32_from_128(vector, slot) (((r32 *)&vector)[slot])

#elif MEKA_X64
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
