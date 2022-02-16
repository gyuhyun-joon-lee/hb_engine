#include <metal_stdlib>

using namespace metal;

#define MEKA_METAL_SHADER
#include "../meka_metal_shader_shared.h"

#if 0
struct LineVertexOutput
{
    float4 clip_p[[position]];
    float3 color;
};
// NOTE(joon) : line vertex function gets a world position input
vertex LineVertexOutput
line_vertex(uint vertexID [[vertex_id]],
                    constant line_vertex *vertices [[buffer(0)]],
                    constant PerFrameData *per_frame_data [[buffer(1)]],
                    constant PerObjectData *per_object_data [[buffer(2)]])
{
    LineVertexOutput result = {};

    float3 world_p = vertices[vertexID].p;

    result.clip_position = per_frame_data->proj_view*convert_to_float4(world_p, 1.0f);
    //output.clip_position = convert_to_float4(world_p, 1.0f);
    //output.clip_position = {10, 0, 0, 1};

    result.color = per_object_data->color;

    return result;
}

fragment float4 
line_frag(LineVertexOutput vertex_output [[stage_in]])
{
    return float4(vertex_output.color, 1.0f);
}
#endif


struct VoxelVertexOutput
{
    float4 clip_p [[position]];

    float3 p [[flat]];
    float3 normal [[flat]];

    float4 color; 
};

// NOTE(joon) : vertex is a prefix for vertex shader
vertex VoxelVertexOutput
voxel_vertex(uint vertexID [[vertex_id]],
                uint instanceID [[instance_id]],
                constant float *vertices [[buffer(0)]], 
                constant PerFrameData *per_frame_data [[buffer(1)]],
                constant float *voxel_positions[[buffer(2)]],
                constant u32 *voxel_colors[[buffer(3)]])
{
    VoxelVertexOutput result = {};

    // TODO(joon) better way of doing this?
    float4 world_p = float4(voxel_positions[3*instanceID] + vertices[3*vertexID+0], 
                            voxel_positions[3*instanceID+1] + vertices[3*vertexID+1],
                            voxel_positions[3*instanceID+2] + vertices[3*vertexID+2],
                            1.0f);

    result.clip_p = per_frame_data->proj_view*world_p;
    result.p = world_p.xyz;
    result.normal = normalize(result.p);

    uint color = voxel_colors[instanceID];
    result.color = float4(round((float)((color >> 0) & 0xff)) / 255.0f,
                        round((float)((color >> 8) & 0xff)) / 255.0f,
                        round((float)((color >> 16) & 0xff)) / 255.0f,
                        round((float)((color >> 24) & 0xff)) / 255.0f);

    return result;
}

fragment float4 
voxel_frag(VoxelVertexOutput vertex_output [[stage_in]])
{
    float4 result = vertex_output.color;

    return result;
}

