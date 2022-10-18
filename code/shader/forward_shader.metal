/*
 * Written by Gyuhyun Lee
 */

#include "shader_common.h"

struct LineVertexOutput
{
    float4 clip_p[[position]];
    float3 color;
};

vertex LineVertexOutput
forward_line_vertex(uint vertex_ID [[vertex_id]],
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
forward_line_frag(LineVertexOutput vertex_output [[stage_in]])
{
    return float4(vertex_output.color, 1.0f);
}

struct RenderPerlinNoiseGridVertexOutput
{
    float4 clip_p [[position]];
    float3 color;
};

vertex RenderPerlinNoiseGridVertexOutput
forward_render_perlin_noise_grid_vert(uint vertexID [[vertex_id]],
                            uint instanceID [[instance_id]],
                             constant float *vertices [[buffer(0)]], 
                             constant float2 *grid_dim [[buffer(1)]],
                             constant float *perlin_values [[buffer(2)]],
                             constant float *floor_z_values [[buffer(3)]],
                             constant uint2 *grass_count [[buffer(4)]],
                             constant PerFrameData *per_frame_data [[buffer(5)]])
{
    RenderPerlinNoiseGridVertexOutput result = {};

    // TODO(gh) instead of hard coding this, any way to get what should be the maximum z value 
    float z = floor_z_values[instanceID] + 5.0f;

    float4 world_p = float4(vertices[2*vertexID+0], 
                                vertices[2*vertexID+1],
                                z,
                                1.0f);

    uint grid_y = instanceID / grass_count->x;
    uint grid_x = instanceID - grid_y * grass_count->x;
    world_p.y += grid_y * grid_dim->y;
    world_p.x += grid_x * grid_dim->x;

    result.clip_p = per_frame_data->proj_view * world_p;

    // TODO(gh) Because perlin noise does not range from 0 to 1, 
    // we need to manually change the value again here - which is another reason
    // why we want the seperation 
    float perlin_value = 2.0f * (perlin_values[instanceID] + 0.25f);
    result.color = float3(perlin_value, perlin_value, perlin_value);

    return result;
}

fragment float4
forward_render_perlin_noise_grid_frag(RenderPerlinNoiseGridVertexOutput vertex_output [[stage_in]])
{
    float4 result = float4(vertex_output.color, 0.7f);
    return result;
}

struct RenderFrustumVetexOutput
{
    float4 clip_p [[position]];
};

vertex RenderFrustumVetexOutput
forward_render_frustum_vert(uint vertexID [[vertex_id]], 
                  constant packed_float3 *vertices [[buffer(0)]],
                  constant float4x4 *proj_view [[buffer(1)]])
{
    RenderFrustumVetexOutput result = {};
    result.clip_p = (*proj_view) * float4(vertices[vertexID], 1.0f);

    return result;
}

fragment float4
forward_render_frustum_frag(RenderFrustumVetexOutput vertex_output [[stage_in]])
{
    float4 result = float4(0, 0, 1, 0.2f);
    return result;
}

struct FontVertexOutput
{
    float4 clip_p [[position]];
    float3 color;
    float2 texcoord;
};

vertex FontVertexOutput
forward_render_font_vertex(uint vertexID [[vertex_id]],
            constant packed_float2 *vertices [[buffer(0)]],
            constant packed_float2 *texcoords [[buffer(1)]],
            constant packed_float3 *color [[buffer(2)]])
{
    FontVertexOutput result = {};

    result.clip_p = float4(vertices[vertexID], 0, 1);
    result.color = *color;
    result.texcoord = texcoords[vertexID];

    return result;
}

fragment float4
forward_render_font_frag(FontVertexOutput vertex_output [[stage_in]],
                        texture2d<uint> font_texture [[texture(0)]]) 
{
    constexpr sampler s = sampler(coord::normalized, address::clamp_to_zero, filter::linear);

    // font bitmap texel is 1 byte, so we will only use x
    // TODO(gh) We can divide the value by 255 here, or normalize it pre-hand
    float4 result = float4(vertex_output.color, font_texture.sample(s, vertex_output.texcoord).x/255.0f);
    //float4 result = float4(vertex_output.color*font_texture.sample(s, vertex_output.texcoord).x/255.0f, 1.0f);

    return result;
}
