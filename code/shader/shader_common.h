#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

#include <metal_stdlib>
using namespace metal;

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

#endif

