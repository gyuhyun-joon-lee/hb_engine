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

struct ScreenSpaceTriangleVertexOutput
{
    float4 clip_p[[position]];
};

vertex ScreenSpaceTriangleVertexOutput
screen_space_triangle_vert(uint vertex_ID [[vertex_id]],
                             constant float *vertices[[buffer(0)]])
{
    ScreenSpaceTriangleVertexOutput result = {};
    result.clip_p = float4(vertices[3*vertex_ID + 0], vertices[3*vertex_ID + 1], vertices[3*vertex_ID + 2], 1.0f);
    
    return result;
}

fragment float4
screen_space_triangle_frag(ScreenSpaceTriangleVertexOutput vertex_output [[stage_in]])
{
    float4 result = float4(1, 0, 0, 1);

    return result;
}



struct VoxelVertexOutput
{
    float4 clip_p [[position]];

    float3 p [[flat]];
    float3 normal [[flat]];

    float4 color; 
};

// NOTE(gh) : vertex is a prefix for vertex shader
vertex VoxelVertexOutput
voxel_vertex(uint vertexID [[vertex_id]],
                uint instanceID [[instance_id]],
                constant float *vertices [[buffer(0)]], 
                constant PerFrameData *per_frame_data [[buffer(1)]],
                constant float *voxel_positions[[buffer(2)]],
                constant uint *voxel_colors[[buffer(3)]])
{
    VoxelVertexOutput result = {};

    // TODO(gh) better way of doing this?
    float4 world_p = float4(voxel_positions[3*instanceID] + vertices[3*vertexID+0], 
                            voxel_positions[3*instanceID+1] + vertices[3*vertexID+1],
                            voxel_positions[3*instanceID+2] + vertices[3*vertexID+2],
                            1.0f);

    result.clip_p = per_frame_data->proj_view * world_p;
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
    float3 color;

    float3 p;
    float3 N;
    float depth;

    float3 p_in_light_coordinate;
};

// TODO(gh) I don't like this...
struct VertexPN
{
    float3 p;
    float3 N;
};

// NOTE(gh) : vertex is a prefix for vertex shader
vertex CubeVertexOutput
singlepass_cube_vertex(uint vertexID [[vertex_id]],
            uint instanceID [[instance_id]],
            constant PerFrameData *per_frame_data [[buffer(0)]],
            constant PerObjectData *per_object_data [[buffer(1)]], 

            // TODO(gh) This is only fine for simple objects with not a lot of vertices
            constant float *positions [[buffer(2)]], 
            constant float *normals [[buffer(3)]],

            // TODO(gh) Make a light buffer so that we can just pass that
            constant float4x4 *light_proj_view[[buffer(4)]])
{
    CubeVertexOutput result = {};

    float4 world_p = per_object_data->model * 
                        float4(positions[3*vertexID+0], 
                                positions[3*vertexID+1],
                                positions[3*vertexID+2],
                                1.0f);

    float4 world_normal = per_object_data->model * 
                        float4(normals[3*vertexID+0], 
                                normals[3*vertexID+1],
                                normals[3*vertexID+2],
                                0.0f);

    result.clip_p = per_frame_data->proj_view * world_p;
    result.color = per_object_data->color;

    result.p = world_p.xyz;
    result.N = normalize(world_normal.xyz);
    result.color = per_object_data->color;
    result.depth = result.clip_p.z / result.clip_p.w;

    // NOTE(gh) because metal doesn't allow to pass the matrix to the fragment shader,
    // we cannot do the shadow lighting on the deferred lighting pass(cannot get the light coordinate position)
    // which is why we are doing this here.
    float4 p_in_light_coordinate = (*light_proj_view) * world_p;
    result.p_in_light_coordinate = p_in_light_coordinate.xyz / p_in_light_coordinate.w;

    // NOTE(gh) Because NDC goes from (-1, -1, 0) to (1, 1, 1) in Metal, we need to change X and Y to range from 0 to 1
    // and flip y, as Metal texture is top-down
    result.p_in_light_coordinate.xy = float2(0.5f, -0.5f) * result.p_in_light_coordinate.xy + float2(0.5f, 0.5f);

    return result;
}



struct ShadowmapVertexOutput
{
    float4 clip_p[[position]];
};

// NOTE(gh) no fragment shader needed
vertex ShadowmapVertexOutput
directional_light_shadowmap_vert(uint vertexID [[vertex_id]],
                                 constant float *positions [[buffer(0)]], 
                                 constant float4x4 *model [[buffer(1)]],
                                 constant float4x4 *proj_view [[buffer(2)]])
{
    ShadowmapVertexOutput result = {};

    result.clip_p = (*proj_view) * (*model) *
                    float4(positions[3*vertexID+0], 
                           positions[3*vertexID+1],
                           positions[3*vertexID+2],
                           1.0f);

    return result;
}



