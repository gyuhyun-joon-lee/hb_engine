/*
 * Written by Gyuhyun Lee
 */

#include "shader_common.h"



// IMPORTANT(gh) vertex always should be PN
vertex GBufferVertexOutput
render_to_g_buffer_vert(uint vertexID [[vertex_id]],
            uint instanceID [[instance_id]],
            constant PerFrameData *per_frame_data [[buffer(0)]],
            constant PerObjectData *per_object_data [[buffer(1)]], 

            // TODO(gh) This is only fine for simple objects with not a lot of vertices
            constant float *vertices [[buffer(2)]], 

            // TODO(gh) Make a light buffer so that we can just pass that
            constant float4x4 *light_proj_view[[buffer(3)]])
{
    GBufferVertexOutput result = {};

    float4 world_p = per_object_data[instanceID].model * 
                        float4(vertices[6*vertexID+0], 
                                vertices[6*vertexID+1],
                                vertices[6*vertexID+2],
                                1.0f);

    float4 world_normal = per_object_data[instanceID].model * 
                        float4(vertices[6*vertexID+3], 
                                vertices[6*vertexID+4],
                                vertices[6*vertexID+5],
                                0.0f);

    result.clip_p = per_frame_data->proj_view * world_p;
    result.color = per_object_data[instanceID].color;

    result.p = world_p.xyz;
    result.N = normalize(world_normal.xyz);
    result.color = per_object_data[instanceID].color;
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
render_to_g_buffer_frag(GBufferVertexOutput vertex_output [[stage_in]],
                    texture2d<float> shadowmap [[texture(0)]])

{
    GBuffers result = {};

    // NOTE(gh) 0 means complete black, 1 means no shadow
    float shadow_factor = 0.0f;
    constexpr sampler shadowmap_sampler = sampler(coord::normalized, address::clamp_to_edge, filter::linear);

    // multisampling shadow
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
            shadow_factor += (shadowmap.sample(shadowmap_sampler, sample_p).x < vertex_output.p_in_light_coordinate.z) ? 0 : 1;
        }
    }

    shadow_factor /= 9.0f; // Because we sampled from 9 different texels

    result.position = float4(vertex_output.p, 0.0f);
    result.normal = float4(vertex_output.N, shadow_factor); // also storing the shadow factor to the unused 4th component
    result.color = float4(vertex_output.color, 1.0f);
    // result.depth = vertex_output.depth; 
   
    return result;
}

struct DeferredLightingVertexOutput
{
    float4 clip_p [[position]];
    float2 texcoord;
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
                                    constant float *light_p[[buffer(0)]],
                                    constant bool *enable_shadow[[buffer(1)]])
{
    DeferredLightingVertexOutput result = {};

    result.clip_p = float4(full_quad_vertices[vertexID], 0.0f, 1.0f);
    result.texcoord = 0.5f*(full_quad_vertices[vertexID] + 1.0f);
    result.texcoord.y = 1.0f - result.texcoord.y;
    result.light_p = float3(light_p[0], light_p[1], light_p[2]);

    result.enable_shadow = *enable_shadow;
    
    return result;
}

fragment float4 
singlepass_deferred_lighting_frag(DeferredLightingVertexOutput vertex_output [[stage_in]],
                       texture2d<float> g_buffer_position [[texture(0)]],
                       texture2d<float> g_buffer_normal [[texture(1)]],
                       texture2d<float> g_buffer_color [[texture(2)]])
{
    constexpr sampler s = sampler(coord::normalized, address::clamp_to_zero, filter::linear);
    float3 p = g_buffer_position.sample(s, vertex_output.texcoord).xyz;
    float4 N = g_buffer_normal.sample(s, vertex_output.texcoord);
    float4 color = g_buffer_color.sample(s, vertex_output.texcoord);

    float3 L = normalize(vertex_output.light_p - p);

    float ambient = 0.2f;

    float shadow_factor = 1.0f;
    if(vertex_output.enable_shadow)
    {
        shadow_factor = N.w; // shadow factor is secretly baked with the normal texture
    }
    float diffuse = shadow_factor * max(dot(N.xyz, L), 0.2f);

    float4 result = float4(color.xyz * (ambient + diffuse), color.w);

    return result;
}
