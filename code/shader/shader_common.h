#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

#include <metal_stdlib>
using namespace metal;

#define INSIDE_METAL_SHADER 1
#include "../hb_shared_with_shader.h"

constant float pi_32 = 3.1415926535897932384626433832795;
constant float tau_32 = 6.283185307179586476925286766559005768394338798750211641949889f;
constant float euler_contant = 2.7182818284590452353602874713526624977572470936999595749f;

// NOTE(gh) original 256 random numbers used by Ken Perlin, ranging from 0 to 255
// TODO(gh) This isn't very effcient by it's nature, I think we can do something better here...
constant uint permutations255[] = 
{ 
    151,160,137,91,90,15,                 
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,    
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

/*
   NOTE(gh)

   High LOD

   15
   / \
   13 12
   |\|
   11 10
   |\|
   9 8
   |\|
   7 6
   |\|
   5 4
   |\|
   3 2
   |\|
   1 0

   Low LOD
    6
   / \
   5 4
   |\|
   3 2
   |\|
   1 0
 */
constant uint grass_high_lod_indices[] = 
{
    0, 3, 1,
    0, 2, 3,

    2, 5, 3,
    2, 4, 5,

    4, 7, 5,
    4, 6, 7,

    6, 9 ,7,
    6, 8, 9,

    8, 11, 9,
    8, 10, 11,

    10, 13, 11,
    10, 12, 13,

    12, 14, 13,
};

constant uint grass_low_lod_indices[] = 
{
    0, 3, 1,
    0, 2, 3,

    2, 5, 3,
    2, 4, 5,

    4, 6, 5,
};

constant float screen_quad[]
{
    -1.0f, -1.0f,
    1.0f, 1.0f,
    -1.0f, 1.0f,

    -1.0f, -1.0f,
    1.0f, -1.0f,
    1.0f, 1.0f,
};

// Vertex shader for populating g buffer
struct GBufferVertexOutput
{
    float4 clip_p [[position]];
    float3 color;

    float3 p;
    float3 N;
    float depth;

    // used to figure out whether the fragment is inside the shadow or not
    float3 p_in_light_coordinate;
};

static float
lerp(float min, float t, float max)
{
    return min + (max-min)*t;
}

#endif

