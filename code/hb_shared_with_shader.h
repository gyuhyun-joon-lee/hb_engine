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
    // TODO(gh) should we use packed_float3?
    float3 color;
#elif INSIDE_VULKAN_SHADER
#else 
    alignas(16) m4x4 model;
    alignas(16) v3 color;
#endif
};

// TODO(gh) any way to handle these better?
/*
   NOTE(gh) Math here is :
   each floor is 10m by 10m wide, we want to plant the grass per 0.02m(2cm) 
   10m / 0.02m = 500, round it up to 512
   512 * 512 = 262144 grasses

   As we cannot make the payload that huge, we will divide the grid into multiple object threadgroups,
   so that each object threadgroup is responsible for 32x16 grasses(thread count per threadgroup should be the same).

   When it comes down to mesh threadgroup, each object threadgroup will fire up 
   object_thread_per_threadgroup_count_x * object_thread_per_threadgroup_count_y amount of mesh threadgroups, 
   so each mesh threadgroup can handle one grass blade. As we have triangle_count_for_one_grass_blade * 3 amount of indices
   per grass, each mesh threadgroup should have that amount of threads.
*/

#define grass_per_grid_count_x 512
#define grass_per_grid_count_y 512
#define grass_high_lod_vertex_count 15
#define grass_high_lod_triangle_count 13
#define grass_high_lod_index_count (grass_high_lod_triangle_count*3)
#define grass_high_lod_divide_count 7

#define grass_low_lod_vertex_count 7
#define grass_low_lod_triangle_count 5
#define grass_low_lod_index_count (grass_low_lod_triangle_count*3)
#define grass_low_lod_divide_count 3

// NOTE(gh) The way we setup how many threads there should be in one object threadgroup
// is based on the payload size limit(16kb) and the simd group width(32)
#define object_thread_per_threadgroup_count_x 32
#define object_thread_per_threadgroup_count_y 8
#define object_threadgroup_per_grid_count_x (grass_per_grid_count_x / object_thread_per_threadgroup_count_x)
#define object_threadgroup_per_grid_count_y (grass_per_grid_count_y / object_thread_per_threadgroup_count_y)

#define mesh_threadgroup_count_x object_thread_per_threadgroup_count_x
#define mesh_threadgroup_count_y object_thread_per_threadgroup_count_y

struct GrassObjectFunctionInput
{
#if INSIDE_METAL_SHADER
    packed_float2 floor_left_bottom_p; // in world unit
    packed_float2 one_thread_worth_dim; // In world unit
#elif INSIDE_VULKAN_SHADER
#else
    alignas(8) v2 floor_left_bottom_p; // in world unit
    alignas(8) v2 one_thread_worth_dim; // in world unit
#endif
};


#endif
