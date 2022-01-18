#include <metal_stdlib>

using namespace metal;

#define MEKA_METAL_SHADER
#include "../meka_metal_shader_shared.h"
#include "../meka_voxel.h"

// Vertex shader outputs and fragment shader inputs
struct vertex_output
{
    // NOTE(joon) [[position]] -> indicator that this is clip space position
    // This is mandatory to be inside the vertex output struct
    r32_4 clip_position [[position]];

    // @NOTE/Joon : These values are in world space
    r32_3 p;
};

// NOTE(joon) : vertex is a prefix for vertex shader
vertex vertex_output
vertex_function(uint vertexID [[vertex_id]],
            const voxel_chunk_hash *voxel_chunk_hashes,
             constant per_frame_data *per_frame_data [[buffer(1)]],
             constant per_object_data *per_object_data [[buffer(2)]])
{
    vertex_output output = {};

    r32_3 model_p = vertices[vertexID].p;
    r32_3 normal = vertices[vertexID].normal;

    r32_3 world_p = (per_object_data->model*convert_to_r32_4(model_p, 1.0f)).xyz;

    r32_4 view_p = per_frame_data->view*convert_to_r32_4(world_p, 1.0f);
    output.clip_position = per_frame_data->proj*view_p;

    output.p = world_p;
    output.normal = normalize((per_object_data->model * convert_to_r32_4(normal, 0.0f)).xyz);
    output.light_p = per_frame_data->light_p;
    output.color = per_object_data->color;

    return output;
}
