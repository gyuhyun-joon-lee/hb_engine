#include "shader_common.h"
#

struct LineVertexOutput
{
    float4 clip_p[[position]];
    float3 color;
};

// NOTE(gh) : line vertex function gets a world position input
// NOTE(gh) line should be rendered in forward pass
vertex LineVertexOutput
singlepass_line_vertex(uint vertex_ID [[vertex_id]],
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
singlepass_line_frag(LineVertexOutput vertex_output [[stage_in]])
{
    // NOTE(gh) This will automatically output to 0th color attachment, which is drawableTexture
    return float4(vertex_output.color, 1.0f);
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


// TODO(gh) I don't like this...
struct VertexPN
{
    float3 p;
    float3 N;
};

// NOTE(gh) : vertex is a prefix for vertex shader
vertex GBufferVertexOutput
singlepass_cube_vertex(uint vertexID [[vertex_id]],
            uint instanceID [[instance_id]],
            constant PerFrameData *per_frame_data [[buffer(0)]],
            constant PerObjectData *per_object_data [[buffer(1)]], 

            // TODO(gh) This is only fine for simple objects with not a lot of vertices
            constant float *vertices [[buffer(2)]], 

            // TODO(gh) Make a light buffer so that we can just pass that
            constant float4x4 *light_proj_view[[buffer(3)]])
{
    GBufferVertexOutput result = {};

    float4 world_p = per_object_data->model * 
                        float4(vertices[6*vertexID+0], 
                                vertices[6*vertexID+1],
                                vertices[6*vertexID+2],
                                1.0f);

    float4 world_normal = per_object_data->model * 
                        float4(vertices[6*vertexID+3], 
                                vertices[6*vertexID+4],
                                vertices[6*vertexID+5],
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

fragment GBuffers 
singlepass_cube_frag(GBufferVertexOutput vertex_output [[stage_in]],
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

    shadow_factor /= 9.0f; // Because we sampled from 9 different texels
#else 
    float shadow_factor = 1.0f;
    // if(vertex_output.p_in_light_coordinate.x <= 1.0f && vertex_output.p_in_light_coordinate.y <= 1.0f)
    {
        // NOTE(gh) If the shadowmap value is smaller(light was closer to another object), this point should be in shadow.
        shadow_factor = 
            shadowmap.sample_compare(shadowmap_sampler, vertex_output.p_in_light_coordinate.xy, vertex_output.p_in_light_coordinate.z);
        // shadow_factor = vertex_output.p_in_light_coordinate.z;
    }
#endif

    result.position = float4(vertex_output.p, 0.0f);
    result.normal = float4(vertex_output.N, shadow_factor); // also storing the shadow factor to the unused 4th component
    result.color = float4(vertex_output.color, 0.0f);
    // result.depth = vertex_output.depth; 
   
    return result;
}

struct DeferredLightingVertexOutput
{
    float4 clip_p [[position]];
    float3 light_p;

    bool enable_shadow;
};

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

vertex DeferredLightingVertexOutput
singlepass_deferred_lighting_vertex(uint vertexID [[vertex_id]],
                // TODO(gh) again, bad idea to pass this one by one...
                         constant float *light_p[[buffer(0)]],
                         constant bool *enable_shadow[[buffer(1)]])
{
    DeferredLightingVertexOutput result = {};

    result.clip_p = float4(full_quad_vertices[vertexID], 0.0f, 1.0f);
    result.light_p = float3(light_p[0], light_p[1], light_p[2]);

    result.enable_shadow = *enable_shadow;
    
    return result;
}

#if __METAL_VERSION__ >= 230
// NOTE(gh) Single pass deferred rendering, which can be identified by the color attachments
// rather than texture binding
fragment float4 
singlepass_deferred_lighting_frag(DeferredLightingVertexOutput vertex_output [[stage_in]],
                       float4 position_g_buffer [[color(1)]],
                       float4 normal_g_buffer [[color(2)]],
                       float4 color_g_buffer [[color(3)]])
{
    float3 p = position_g_buffer.xyz;
    float4 N = normal_g_buffer;
    float3 color = color_g_buffer.xyz;

    float3 L = normalize(vertex_output.light_p - p);

    float ambient = 0.2f;

    float shadow_factor = 1.0f;
    if(vertex_output.enable_shadow)
    {
        shadow_factor = N.w; // shadow factor is secretly baked with the normal texture
    }
    float diffuse = shadow_factor * max(dot(N.xyz, L), 0.0f);

    float4 result = float4(color * (ambient + diffuse), 1.0f);

    return result;
}
#else
// TODO(gh) traditional deferred lighting pass?
#endif
