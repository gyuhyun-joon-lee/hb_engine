#ifndef HB_SHARED_WITH_SHADER_H
#define HB_SHARED_WITH_SHADER_H

struct PerFrameData
{
#if INSIDE_METAL_SHADER
    float4x4 proj_view;
#elif INSIDE_VULKAN_SHADER
#else
    alignas(16) m4x4 proj_view;
#endif
};

struct PerObjectData
{
#if INSIDE_METAL_SHADER
    float4x4 model;
    // TODO(gh) should we use packed_float3?
    float3 color;
#elif INSIDE_VULKAN_SHADER
#else 
    alignas(16) m4x4 model;
    alignas(16) v3 color;
#endif
};

#endif
