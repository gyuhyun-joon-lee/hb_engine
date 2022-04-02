#include <metal_stdlib>

using namespace metal;

// NOTE(joon) this is a consequence of not having a shared file
// between metal and the platform code.. which is a rabbit hole
// that I do not want to go inside :(
struct PerFrameData
{
    float4 proj_view[4];
};

struct PerObjectData
{
    float4 model[4];
    float4 color;
};


struct LineVertexOutput
{
    float4 clip_p[[position]];
    float3 color;
};

// NOTE(joon) : line vertex function gets a world position input
vertex LineVertexOutput
line_vertex(uint vertex_ID [[vertex_id]],
                    constant float *vertices [[buffer(0)]],
                    constant PerFrameData *per_frame_data [[buffer(1)]],
                    constant float *color [[buffer(2)]])
{
    LineVertexOutput result = {};

    float3 world_p = float3(vertices[3*vertex_ID + 0], vertices[3*vertex_ID + 1], vertices[3*vertex_ID + 2]);

    result.clip_p = (*(constant float4x4 *)&(per_frame_data->proj_view)) * float4(world_p, 1.0f);

    result.color = *(constant float3 *)color;

    return result;
}

fragment float4 
line_frag(LineVertexOutput vertex_output [[stage_in]])
{
    return float4(vertex_output.color, 1.0f);
}


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
                constant uint *voxel_colors[[buffer(3)]])
{
    VoxelVertexOutput result = {};

    // TODO(joon) better way of doing this?
    float4 world_p = float4(voxel_positions[3*instanceID] + vertices[3*vertexID+0], 
                            voxel_positions[3*instanceID+1] + vertices[3*vertexID+1],
                            voxel_positions[3*instanceID+2] + vertices[3*vertexID+2],
                            1.0f);

    result.clip_p = (*(constant float4x4 *)&(per_frame_data->proj_view)) * world_p;
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

struct CubeVertexOutput
{
    float4 clip_p [[position]];
    float4 color;
};

// NOTE(joon) : vertex is a prefix for vertex shader
vertex CubeVertexOutput
cube_vertex(uint vertexID [[vertex_id]],
                uint instanceID [[instance_id]],
                constant float *vertices [[buffer(0)]], 
                constant PerFrameData *per_frame_data [[buffer(1)]],
                constant PerObjectData *per_object_data [[buffer(2)]])
{
    CubeVertexOutput result = {};

    float4 world_p = *(constant float4x4 *)&(per_object_data->model) * 
                        float4(vertices[3*vertexID+0], 
                                vertices[3*vertexID+1],
                                vertices[3*vertexID+2],
                                1.0f);

    result.clip_p = (*(constant float4x4 *)&(per_frame_data->proj_view)) * world_p;
    //result.color = float4(per_object_data->color.r, 
                          //per_object_data->color.g, 
                          //per_object_data->color.b, 
                          //1.0f);
    result.color = float4(1, 0, 0, 1);

    return result;
}

fragment float4 
cube_frag(CubeVertexOutput vertex_output [[stage_in]])
{
    float4 result = vertex_output.color;

    return result;
}

