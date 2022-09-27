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

struct GrassObjectFunctionInput
{
#if INSIDE_METAL_SHADER
    packed_float2 one_thread_worth_dim; // In world unit
    uint32_t mesh_threadgroup_count_x; // object thread count
    uint32_t mesh_threadgroup_count_y; // 
#elif INSIDE_VULKAN_SHADER
#else
    alignas(4) v2 one_thread_worth_dim;
    alignas(4) u32 mesh_threadgroup_count_x;
    alignas(4) u32 mesh_threadgroup_count_y;
#endif
};


#endif
