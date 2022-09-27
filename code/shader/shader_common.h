#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

#include <metal_stdlib>
using namespace metal;

#define INSIDE_METAL_SHADER 1
#include "../hb_shared_with_shader.h"

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef int32_t b32;

typedef uint8_t u8; 
typedef uint16_t u16; 
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;

constant float pi_32 = 3.1415926535897932384626433832795;


// Populating g buffer
struct GBufferVertexOutput
{
    float4 clip_p [[position]];
    float3 color;

    float3 p;
    float3 N;
    float depth;

    // used to figure out whether the fragment is inside the shadow or not
    float3 p_in_light_coordinate;
};

#endif

