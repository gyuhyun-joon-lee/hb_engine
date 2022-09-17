#include <metal_stdlib>

using namespace metal;

struct LineVertexOutput
{
    float4 clip_p[[position]];
    float3 color;
};

// NOTE(gh) : line vertex function gets a world position input
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

fragment GBuffers 
singlepass_cube_frag(CubeVertexOutput vertex_output [[stage_in]],
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

struct DeferredLightingVertexOutput
{
    float4 clip_p [[position]];
    float3 light_p;
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

    float shadow_factor = N.w; // shadow factor is secretly baked with the normal texture
    float diffuse = shadow_factor * max(dot(N.xyz, L), 0.0f);

    float4 result = float4(color * (ambient + diffuse), 1.0f);

    return result;
}
#else
// TODO(gh) traditional deferred lighting pass?
#endif
