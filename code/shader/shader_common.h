#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

#include <metal_stdlib>
using namespace metal;

constant float pi_32 = 3.1415926535897932384626433832795;

// NOTE(gh) this is a consequence of not having a shared file
// between metal and the platform code.. which is a rabbit hole
// that I do not want to go inside :(
struct PerFrameData
{
    float4x4 proj_view;
};

struct PerObjectData
{
    float4x4 model;
    float3 color;
};

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

