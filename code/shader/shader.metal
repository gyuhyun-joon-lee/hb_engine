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

struct LineVertexOutput
{
    float4 clip_p[[position]];
    float3 color;
};

// NOTE(gh) : line vertex function gets a world position input
vertex LineVertexOutput
line_vertex(uint vertex_ID [[vertex_id]],
            constant PerFrameData *per_frame_data [[buffer(0)]],
            constant float *vertices [[buffer(1)]],
            constant float *color [[buffer(2)]])
{
    LineVertexOutput result = {};

    float3 world_p = float3(vertices[3*vertex_ID + 0], vertices[3*vertex_ID + 1], vertices[3*vertex_ID + 2]);

    result.clip_p = per_frame_data->proj_view * float4(world_p, 1.0f);

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
};

// TODO(gh) I don't like this...
struct VertexPN
{
    float3 p;
    float3 N;
};

// NOTE(gh) : vertex is a prefix for vertex shader
vertex CubeVertexOutput
cube_vertex(uint vertexID [[vertex_id]],
                uint instanceID [[instance_id]],
                constant PerFrameData *per_frame_data [[buffer(0)]],
                constant PerObjectData *per_object_data [[buffer(1)]], 

                // TODO(gh) This is fine, as long as we will going to use instacing to draw the cubes
                constant float *positions [[buffer(2)]], 
                constant float *normals [[buffer(3)]])
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

    return result;
}

// NOTE(gh) This will be outputted by the deferred pass before the lighting pass
struct GBuffers
{
    // TODO(gh) maybe this is too big... shrink these down by sacrificing precisions
    float4 position [[color(0)]];
    float4 normal [[color(1)]];
    float4 color [[color(2)]];

    float depth [[color(3)]];
};


fragment GBuffers 
cube_frag(CubeVertexOutput vertex_output [[stage_in]])
{
    GBuffers result = {};

    result.position = float4(vertex_output.p, 0.0f);
    result.normal = float4(vertex_output.N, 0.0f);
    result.color = float4(vertex_output.color, 0.0f);
    result.depth = vertex_output.clip_p.z;
   
    return result;
}

// NOTE(gh) This generates one full quad, assuming that the vertexID is 0, 1, 2 ... 5
constant float2 full_quad_vertices[]
{
    {-1.0f, -1.0f},
    {1.0f, 1.0f},
    {-1.0f, 1.0f},

    {-1.0f, -1.0f},
    {1.0f, -1.0f},
    {1.0f, 1.0f},
};

struct DeferredLightingVertexOutput
{
    float4 clip_p [[position]];

    // TODO(gh) just pass the perframedata to the fragment function
    float3 light_p;
};

vertex DeferredLightingVertexOutput
deferred_lighting_vertex(uint vertexID [[vertex_id]])
{
    DeferredLightingVertexOutput result = {};

    result.clip_p = float4(full_quad_vertices[vertexID], 0.0f, 1.0f);

    result.light_p = float3(100.0f, 100.0f, 2.0f);

    return result;
}

fragment float4 
deferred_lighting_frag(DeferredLightingVertexOutput vertex_output [[stage_in]],
                       texture2d<float> position_g_buffer [[texture(0)]],
                       texture2d<float> normal_g_buffer [[texture(1)]],
                       texture2d<float> color_g_buffer [[texture(2)]], 
                       texture2d<float> depth_g_buffer [[texture(3)]])
{
    // NOTE(gh) This is what Apple is doing in their deferred rendering sample
    uint2 sample_p = uint2(vertex_output.clip_p.xy);

    float3 p = position_g_buffer.read(sample_p).xyz;
    float3 N = normal_g_buffer.read(sample_p).xyz;
    float3 color = color_g_buffer.read(sample_p).xyz;
    // float depth = depth_g_buffer.read(sample_p).x;

    float3 L = normalize(vertex_output.light_p - p);

    float ambient = 0.2f;
    float diffuse = max(dot(N, L), 0.1f);

    float4 result = float4(color * (ambient + diffuse), 1.0f);

    return result;
}
