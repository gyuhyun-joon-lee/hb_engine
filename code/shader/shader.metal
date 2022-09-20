#include "shader_common.h"

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

struct ShadowmapVertexOutput
{
    float4 clip_p[[position]];
};

// NOTE(gh) no fragment shader needed
vertex ShadowmapVertexOutput
directional_light_shadowmap_vert(uint vertexID [[vertex_id]],
                                 constant float *vertices [[buffer(0)]], 
                                 constant float4x4 *model [[buffer(1)]],
                                 constant float4x4 *proj_view [[buffer(2)]])
{
    ShadowmapVertexOutput result = {};

    // NOTE(gh) For now, the vertex has position + normal, which is why
    // the stride is 6 floats
    result.clip_p = (*proj_view) * (*model) *
                    float4(vertices[6*vertexID+0], 
                           vertices[6*vertexID+1],
                           vertices[6*vertexID+2],
                           1.0f);

    return result;
}



