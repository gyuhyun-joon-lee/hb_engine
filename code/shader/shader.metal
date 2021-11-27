#include <metal_stdlib>

using namespace metal;

#define MEKA_LLVM 1
#define MEKA_METAL 1
#include "../meka_metal_shader_shared.h"

// Vertex shader outputs and fragment shader inputs
struct vertex_output
{
    // @NOTE/Joon: [[position]] -> indicator that this is clip space position
    // This is mandatory to be inside the vertex output struct
    r32_4 clip_position [[position]];

    // @NOTE/Joon : These values are in world space
    r32_3 p;
    r32_3 normal;
    r32_3 light_p;
};

// NOTE(joon) : vertex is a prefix for vertex shader
vertex vertex_output
vertex_function(uint vertexID [[vertex_id]],
             constant temp_vertex *vertices [[buffer(0)]],
             constant r32_2 *viewportSizePointer [[buffer(1)]],
             constant frame_data *per_frame_data [[buffer(2)]])
{
    vertex_output output;

    r32_3 model_p = vertices[vertexID].p;
    r32_3 normal = vertices[vertexID].normal;

    // TODO/joon : This is so werid...
    r32_3 world_p = convert_to_r32_3(per_frame_data->model*convert_to_r32_4(model_p, 1.0f));

    output.clip_position = per_frame_data->proj_view*convert_to_r32_4(world_p, 1.0f);
    output.p = world_p;
    output.normal = convert_to_r32_3(per_frame_data->model * convert_to_r32_4(normal, 0.0f));
    output.light_p = per_frame_data->light_p;

    return output;
}

fragment r32_4 
frag_phong_lighting(vertex_output vertex_output [[stage_in]])
{
    r32_3 light_color = {1, 1, 1};
    r32_3 light_dir = normalize(vertex_output.light_p - vertex_output.p.xyz);

    //float distanceSquare = dot(vertex_output.light_p, vertex_output.p.xyz);
    //float attenuation = 10000.0f/distanceSquare;

    r32_3 ambient = light_color * 0.0f;
    r32_3 diffuse = light_color*max(dot(light_dir, vertex_output.normal), 0.0f);

    r32_3 result_color = ambient + diffuse;
    r32_4 out_color = convert_to_r32_4(result_color, 1.0f);

    return out_color;
}

