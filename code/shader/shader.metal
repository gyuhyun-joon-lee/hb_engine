#include <metal_stdlib>

using namespace metal;

#define MEKA_METAL_SHADER
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
    r32_3 color;
};

// NOTE(joon) : vertex is a prefix for vertex shader
vertex vertex_output
vertex_function(uint vertexID [[vertex_id]],
             constant temp_vertex *vertices [[buffer(0)]], 
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

fragment r32_4 
phong_frag_function(vertex_output vertex_output [[stage_in]])
{
    r32_3 light_color = {1, 1, 1};
    r32_3 light_dir = normalize(vertex_output.light_p - vertex_output.p.xyz);

    //float distanceSquare = dot(vertex_output.light_p, vertex_output.p.xyz);
    //float attenuation = 10000.0f/distanceSquare;

    r32_3 ambient = light_color * 0.0f;
    r32_3 diffuse = light_color*max(dot(light_dir, vertex_output.normal), 0.0f);

    r32_3 result_color = ambient + diffuse;
    r32_4 out_color = convert_to_r32_4(result_color, 1.0f);
    //out_color = 2.f*R32_4(48/255.0f, 10/255.0f, 36/255.0f, 1);

    return out_color;
}

// NOTE(joon): Not exactly doing the raytracing, just rendering the raytraced objects
// NOTE(joon): This also receives a world coordinate position as an input
vertex vertex_output
raytracing_vertex_function(uint vertexID [[vertex_id]],
                            constant r32_3 *vertices [[buffer(0)]],
                            constant per_frame_data *per_frame_data [[buffer(1)]],
                            constant per_object_data *per_object_data [[buffer(2)]])
{
    vertex_output output = {};

    r32_3 world_p = vertices[vertexID].xyz;

    output.clip_position = per_frame_data->proj_view*convert_to_r32_4(world_p, 1.0f);

    output.color = per_object_data->color;

    return output;
}

fragment r32_4 
raytracing_frag_function(vertex_output vertex_output [[stage_in]])
{
    return convert_to_r32_4(vertex_output.color, 1.0f);
}

struct line_vertex_output
{
    r32_4 clip_position [[position]];

    r32_3 p;
    r32_3 color;
};

// NOTE(joon) : line vertex function gets a world position input
vertex line_vertex_output
line_vertex_function(uint vertexID [[vertex_id]],
                    constant line_vertex *vertices [[buffer(0)]],
                    constant per_frame_data *per_frame_data [[buffer(1)]],
                    constant per_object_data *per_object_data [[buffer(2)]])
{
    line_vertex_output output = {};

    r32_3 world_p = vertices[vertexID].p;

    output.clip_position = per_frame_data->proj_view*convert_to_r32_4(world_p, 1.0f);

    output.color = per_object_data->color;

    return output;
}

fragment r32_4 
line_frag_function(vertex_output vertex_output [[stage_in]])
{
    return convert_to_r32_4(vertex_output.color, 1.0f);
}

