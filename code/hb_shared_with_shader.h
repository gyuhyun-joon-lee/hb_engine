/*
 * Written by Gyuhyun Lee
 */

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
    float3 color;
#elif INSIDE_VULKAN_SHADER
#else 
    alignas(16) m4x4 model;
    alignas(16) v3 color;
#endif
};

// TODO(gh) any way to handle these better?
/*
    TERMINOLOGY
    thread : a single instance of the shader. Includes registers, ID.
    threadgroup : Equivalent to the workgroup. bunch of threads. Only one CU will take the threadgroup and do the work. 
                  For example, two CU cannot split a single threadgroup. Threads in the same thread group shares same thread memory.
    SIMDgroup : Equivalent to wavefront or warp. Threads in the same SIMDgroup will be executed with same instruction set 
                in lock-step. Threads in the same SIMDgroup cannot diverge, and CU cannot switch to individual thread -
                it can only switch to entirely new SIMDgroup to hide latency. When the GPU collects the threads into SIMDgroup,
                it can try to do that based on the divergence(for example, one for the A routine of the if statement
                and another one for the b routine)
*/

#define grass_high_lod_vertex_count 15
#define grass_high_lod_triangle_count 13
#define grass_high_lod_index_count (grass_high_lod_triangle_count*3)
#define grass_high_lod_divide_count 7

#define grass_low_lod_vertex_count 7
#define grass_low_lod_triangle_count 5
#define grass_low_lod_index_count (grass_low_lod_triangle_count*3)
#define grass_low_lod_divide_count 3

// NOTE(gh) The way we setup how many threads there should be in one object threadgroup
// is based on the payload size limit(16kb) and the simd group width(32).
// The threadgruop count per grid will be determined by the thread count, and it's up to the platform layer
// to decide the size of one wavefront.
// TODO(gh) It seems like we cannot increase this value too much because of the command buffer overflow,
// figure something out(or should we?)?
#define object_thread_per_threadgroup_count_x 16
#define object_thread_per_threadgroup_count_y 16

struct GrassObjectFunctionInput
{
#if INSIDE_METAL_SHADER
    packed_float2 min; // in world unit
    packed_float2 max; // in world unit
    packed_float2 one_thread_worth_dim; // In world unit
    // packed_float2 floor_center;
    // packed_float2 floor_half_dim;
#elif INSIDE_VULKAN_SHADER
#else
    alignas(8) v2 min; // in world unit
    alignas(8) v2 max; // in world unit
    alignas(8) v2 one_thread_worth_dim; // in world unit
    // alignas(8) v2 floor_center;
    //alignas(8) v2 floor_half_dim;
#endif
};

struct GridInfo
{
#if INSIDE_METAL_SHADER
    packed_float2 min; // in world unit
    packed_float2 max; // in world unit
    packed_float2 one_thread_worth_dim; // in world unit
#elif INSIDE_VULKAN_SHADER
#else
    alignas(8) v2 min; // in world unit
    alignas(8) v2 max; // in world unit
    alignas(8) v2 one_thread_worth_dim; // in world unit
#endif
};

/*
    Original implementation : 64 bytes
    packed_float3 center;

    uint hash;
    float blade_width; // can be a half
    float length; // can be a half
    float tilt; // can be a half
    packed_float2 facing_direction;
    float bend; // can be a half
    float wiggliness;
    packed_float3 color;
    float pad[2];
*/
// 
struct GrassInstanceData
{
#if INSIDE_METAL_SHADER
#if 0

#else
    packed_float3 p0;
    packed_float3 p1;
    packed_float3 p2;

    packed_float3 v0;
    packed_float3 v1;
    packed_float3 v2;

    // TODO(gh) This can be calculated from the vertex shader using facing direction,
    // which is cos(hash) sin(hash)
    packed_float3 orthogonal_normal;
    uint hash;

    packed_half3 color;
    half wiggliness;

    half blade_width;
#endif
#elif INSIDE_VULKAN_SHADER
#else
    f32 p0[3];
    f32 p1[3];
    f32 p2[3];

    f32 v0[3];
    f32 v1[3];
    f32 v2[3];

    f32 orthogonal_normal[3];
    u32 hash;

    f32 color_wig[2];
    f32 pad;
#endif
};

// TODO(gh) We'll probably need something more generic than this 
// like capsule, box..
struct SphereInfo
{
#if INSIDE_METAL_SHADER
    packed_float3 center;
    float r;
#elif INSIDE_VULKAN_SHADER
#else 
    v3 center;
    f32 r;
#endif
};

#endif
