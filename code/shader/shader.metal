/*
 * Written by Gyuhyun Lee
 */

#include "shader_common.h"


float
get_gradient_value(uint hash, float x, float y, float z)
{
    float result = 0.0f;

    /*
       NOTE(gh) This essentially picks a random vector from
        {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
        {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
        {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}

        and then dot product with the distance vector
    */
    switch(hash & 0xF)
    {
        // TODO(gh) Can we do something with the diverging?
        case 0x0: result = x + y; break;
        case 0x1: result = -x + y; break;
        case 0x2: result =  x - y; break;
        case 0x3: result = -x - y; break;
        case 0x4: result =  x + z; break;
        case 0x5: result = -x + z; break;
        case 0x6: result =  x - z; break;
        case 0x7: result = -x - z; break;
        case 0x8: result =  y + z; break;
        case 0x9: result = -y + z; break;
        case 0xA: result = y - z; break;
        case 0xB: result = -y - z; break;
        case 0xC: result =  y + x; break;
        case 0xD: result = -y + z; break;
        case 0xE: result =  y - x; break;
        case 0xF: result = -y - z; break;
    }

    return result;
}

float
fade(float t) 
{
    return t*t*t*(t*(t*6 - 15)+10);
}

float 
perlin_noise01(float x, float y, float z, uint frequency)
{
    uint xi = floor(x);
    uint yi = floor(y);
    uint zi = floor(z);
    uint x255 = xi % frequency; // used for hashing from the random numbers
    uint y255 = yi % frequency; // used for hashing from the random numbers
    uint z255 = zi % frequency;

    float xf = x - (float)xi; // fraction part of x, should be in 0 to 1 range
    float yf = y - (float)yi; // fraction part of y, should be in 0 to 1 range
    float zf = z - (float)zi;

    float u = fade(xf);
    float v = fade(yf);
    float w = fade(zf);

    /*
       NOTE(gh) naming scheme
       2      3
       --------
       |      |
       |      |
       |      |
       --------
       0      1

       // +z
       6      7
       --------
       |      |
       |      |
       |      |
       --------
       4      5

       So for example, gradient vector 0 is a gradient vector used for the point 0, 
       and gradient 0 is the the graident value for point 0.
       The positions themselves do not matter that much, but the relative positions to each other
       are important as we need to interpolate in x, y, (and possibly z or w) directions.
    */
     
    // TODO(gh) We need to do this for now, because the perlin noise is per grid basis,
    // which means the edge of grid 0 should be using the same value as the start of grid 1.
    uint x255_inc = (x255+1)%frequency;
    uint y255_inc = (y255+1)%frequency;
    uint z255_inc = (z255+1)%frequency;

    int random_value0 = permutations255[(permutations255[(permutations255[x255]+y255)%256]+z255)%256];
    int random_value1 = permutations255[(permutations255[(permutations255[x255_inc]+y255)%256]+z255)%256];
    int random_value2 = permutations255[(permutations255[(permutations255[x255]+y255_inc)%256]+z255)%256];
    int random_value3 = permutations255[(permutations255[(permutations255[x255_inc]+y255_inc)%256]+z255)%256];

    int random_value4 = permutations255[(permutations255[(permutations255[x255]+y255)%256]+z255_inc)%256];
    int random_value5 = permutations255[(permutations255[(permutations255[x255_inc]+y255)%256]+z255_inc)%256];
    int random_value6 = permutations255[(permutations255[(permutations255[x255]+y255_inc)%256]+z255_inc)%256];
    int random_value7 = permutations255[(permutations255[(permutations255[x255_inc]+y255_inc)%256]+z255_inc)%256];

    // NOTE(gh) -1 are for getting the distance vector
    float gradient0 = get_gradient_value(random_value0, xf, yf, zf);
    float gradient1 = get_gradient_value(random_value1, xf-1, yf, zf);
    float gradient2 = get_gradient_value(random_value2, xf, yf-1, zf);
    float gradient3 = get_gradient_value(random_value3, xf-1, yf-1, zf);

    float gradient4 = get_gradient_value(random_value4, xf, yf, zf-1);
    float gradient5 = get_gradient_value(random_value5, xf-1, yf, zf-1);
    float gradient6 = get_gradient_value(random_value6, xf, yf-1, zf-1);
    float gradient7 = get_gradient_value(random_value7, xf-1, yf-1, zf-1);

    float lerp01 = lerp(gradient0, u, gradient1); // lerp between 0 and 1
    float lerp23 = lerp(gradient2, u, gradient3); // lerp between 2 and 3 
    float lerp0123 = lerp(lerp01, v, lerp23); // lerp between '0-1' and '2-3', in y direction

    float lerp45 = lerp(gradient4, u, gradient5); // lerp between 4 and 5
    float lerp67 = lerp(gradient6, u, gradient7); // lerp between 6 and 7 
    float lerp4567 = lerp(lerp45, v, lerp67); // lerp between '4-5' and '6-7', in y direction

    float lerp01234567 = lerp(lerp0123, w, lerp4567);

    float result = (lerp01234567 + 1.0f) * 0.5f; // put into 0 to 1 range
    // float result = (lerp0123 + 1.0f) * 0.5f; // put into 0 to 1 range

    return result;
}

struct GenerateWindNoiseVertexOutput
{
    float4 clip_p[[position]];
    float3 p0to1;

    uint layer [[render_target_array_index]];
};

vertex GenerateWindNoiseVertexOutput
generate_wind_noise_vert(uint vertexID [[vertex_id]],
                        constant uint *layer [[buffer(0)]],
                        constant uint *max_layer [[buffer(1)]])
{
    GenerateWindNoiseVertexOutput result = {};
    result.clip_p = float4(screen_quad[2*vertexID + 0], screen_quad[2*vertexID + 1], 0.0f, 1.0f);
    result.layer = *layer;
    result.p0to1 = float3(0.5f*result.clip_p.xy + float2(1, 1), result.layer/(float)*max_layer);
    
    return result;
}

fragment float4
generate_wind_noise_frag(GenerateWindNoiseVertexOutput vertex_output [[stage_in]])
{
    // NOTE(gh) starting from 8 or 16 seems like working well.
    float factor = 16.0f;
    float weight = 0.5f;

    float4 result = float4(0, 0, 0, 0);
    for(uint i = 0;
            i < 16;
            ++i)
    {
        float x = factor * vertex_output.p0to1.x; 
        float y = factor * vertex_output.p0to1.y; 
        float z = factor * vertex_output.p0to1.z;

        result += weight * float4(perlin_noise01(x, y, z, factor), 0, 0, 0);

        factor *= 2;
        weight *= 0.5f;
    }

    return result;
}

struct ScreenSpaceTriangleVertexOutput
{
    float4 clip_p[[position]];
    float2 p0to1;
    float time_elasped;
};

vertex ScreenSpaceTriangleVertexOutput
screen_space_triangle_vert(uint vertex_ID [[vertex_id]],
                             constant float *vertices [[buffer(0)]],
                             constant float *time_elasped [[buffer(1)]])
{
    ScreenSpaceTriangleVertexOutput result = {};
    result.clip_p = float4(vertices[3*vertex_ID + 0], vertices[3*vertex_ID + 1], vertices[3*vertex_ID + 2], 1.0f);
    result.p0to1 = 0.5f*(result.clip_p.xy/result.clip_p.w) + float2(1, 1);
    result.time_elasped = *time_elasped;
    
    return result;
}

fragment float4
screen_space_triangle_frag(ScreenSpaceTriangleVertexOutput vertex_output [[stage_in]])
{
    float factor = 16.0f;
    float x = factor * vertex_output.p0to1.x; 
    float y = factor * vertex_output.p0to1.y; 
    float perlin_noise_value = perlin_noise01(x, y, 0.0f, 32);

    float4 result = float4(perlin_noise_value, perlin_noise_value, perlin_noise_value, 0.2f);

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


uint xor_shift(uint x, uint y, uint z)
{    
    // TODO(gh) better hash key lol
    uint key = 123*x+332*y+395*z;
    key ^= (key << 13);
    key ^= (key >> 17);    
    key ^= (key << 5);    
    return key;
}


