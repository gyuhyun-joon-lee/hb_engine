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
cube_vertex(uint vertexID [[vertex_id]],
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

struct GBuffers
{
    // TODO(gh) maybe this is too big... shrink these down by sacrificing precisions
    // NOTE(gh) This starts from 1, because if we wanna use single pass deferred rendering,
    // the deferred lighting pass(which outputs the final color into the color attachment 0)
    // should use the same numbers as these.
    // so for the g buffer renderpass, colorattachment 0 will be specifed as invalid.
    // and we will not output anything to that.
    float4 position [[color(1)]];
    float4 normal [[color(2)]];
    float4 color [[color(3)]];
};

fragment GBuffers 
cube_frag(CubeVertexOutput vertex_output [[stage_in]],
          depth2d<float> shadowmap [[texture(0)]],
          sampler shadowmap_sampler[[sampler(0)]])
{
    GBuffers result = {};

#if 1
    // multisampling shadow
    float shadow_factor = 0.0f;
    float one_over_shadowmap_texture_width = 1.0f / shadowmap.get_width();
    float one_over_shadowmap_texture_height = 1.0f / shadowmap.get_height();

    for(int y = -1;
            y <= 1;
            ++y)
    {
        for(int x = -1;
                x <= 1;
                ++x)
        {
            float2 sample_p = vertex_output.p_in_light_coordinate.xy + float2(x, y) * float2(one_over_shadowmap_texture_width, one_over_shadowmap_texture_height);
            shadow_factor += shadowmap.sample_compare(shadowmap_sampler, sample_p, vertex_output.p_in_light_coordinate.z);
        }
    }

    shadow_factor /= 9; // Because we sampled from 9 different texels
#else 
    float shadow_factor = 1.0f;
    // if(vertex_output.p_in_light_coordinate.x <= 1.0f && vertex_output.p_in_light_coordinate.y <= 1.0f)
    {
        // NOTE(gh) If the shadowmap value is smaller(light was closer to another object), this point should be in shadow.
        shadow_factor = 
            shadowmap.sample_compare(shadowmap_sampler, vertex_output.p_in_light_coordinate.xy, vertex_output.p_in_light_coordinate.z);
    }
#endif

    result.position = float4(vertex_output.p, 0.0f);
    result.normal = float4(vertex_output.N, shadow_factor); // also storing the shadow factor to the unused 4th component
    result.color = float4(vertex_output.color, 0.0f);
    // result.depth = vertex_output.depth; 
   
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
    float3 light_p;
};

vertex DeferredLightingVertexOutput
deferred_lighting_vertex(uint vertexID [[vertex_id]],
                // TODO(gh) again, bad idea to pass this one by one...
                         constant float *light_p[[buffer(0)]])
{
    DeferredLightingVertexOutput result = {};

    result.clip_p = float4(full_quad_vertices[vertexID], 0.0f, 1.0f);
    result.light_p = float3(light_p[0], light_p[1], light_p[2]);
    
    return result;
}

#if __METAL_VERSION__ >= 230
// NOTE(gh) Single pass deferred rendering, which can be identified by the color attachments
// rather than texture binding
fragment float4 
deferred_lighting_frag(DeferredLightingVertexOutput vertex_output [[stage_in]],
                       float4 position_g_buffer [[color(1)]],
                       float4 normal_g_buffer [[color(2)]],
                       float4 color_g_buffer [[color(3)]])
{
    float3 p = position_g_buffer.xyz;
    float4 N = normal_g_buffer;
    float3 color = color_g_buffer.xyz;

    float3 L = normalize(vertex_output.light_p - p);

    float ambient = 0.2f;

    float shadow_factor = N.w; // shadow factor is secretly baked with the normal texture
    float diffuse = shadow_factor * max(dot(N.xyz, L), 0.0f);

    float4 result = float4(color * (ambient + diffuse), 1.0f);

    return result;
}
#else
// TODO(gh) traditional deferred lighting pass?
#endif
