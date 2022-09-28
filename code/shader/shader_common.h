#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

#include <metal_stdlib>
using namespace metal;

#define INSIDE_METAL_SHADER 1
#include "../hb_shared_with_shader.h"

constant float pi_32 = 3.1415926535897932384626433832795;
constant float tau_32 = 6.283185307179586476925286766559005768394338798750211641949889f;

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

