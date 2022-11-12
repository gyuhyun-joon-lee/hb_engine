/*
 * Written by Gyuhyun Lee
 */

#include "shader_common.h"

static float
lerp(float min, float t, float max)
{
    return min + (max-min)*t;
}

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

// From one the the Apple's sample shader codes - 
// https://developer.apple.com/library/archive/samplecode/MetalShaderShowcase/Listings/MetalShaderShowcase_AAPLWoodShader_metal.html
float random_between_0_1(int x, int y, int z)
{
    int seed = x + y * 57 + z * 241;
    seed = (seed << 13) ^ seed;
    return (( 1.0 - ( (seed * (seed * seed * 15731 + 789221) + 1376312589) & 2147483647) / 1073741824.0f) + 1.0f) / 2.0f;
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

uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

// 16 floats = 64 bytes
struct PerGrassData
{
    // TODO(gh) This works, but only if we make the x and y values as constant 
    // and define them somewhere, which is unfortunate.
    packed_float3 center;

    // TODO(gh) I would assume that I can also add per instance data here,
    // but there might be a more efficient way to do this.
    uint hash;
    float blade_width;
    float length;
    float tilt;
    packed_float2 facing_direction;
    float bend;
    float wiggliness;
    packed_float3 color;
    float time_elasped_from_start; // TODO(gh) Do we even need to pass this?
    float pad;
};

// NOTE(gh) each payload should be less than 16kb
struct Payload
{
    PerGrassData per_grass_data[object_thread_per_threadgroup_count_x * object_thread_per_threadgroup_count_y];
};

bool
is_inside_cube(packed_float3 p, packed_float3 min, packed_float3 max)
{
    bool result = true;
    if(p.x < min.x || p.x > max.x ||
        p.y < min.y || p.y > max.y ||    
        p.z < min.z || p.z > max.z)
    {
        result = false;
    }

    return result;
}

// TODO(gh) Should be more conservative with the value,
// and also change this to seperating axis test 
bool 
is_inside_frustum(constant float4x4 *proj_view, float3 min, float3 max)
{
    bool result = false;

    float4 vertices[8] = 
    {
        // bottom
        float4(min.x, min.y, min.z, 1.0f),
        float4(min.x, max.y, min.z, 1.0f),
        float4(max.x, min.y, min.z, 1.0f),
        float4(max.x, max.y, min.z, 1.0f),

        // top
        float4(min.x, min.y, max.z, 1.0f),
        float4(min.x, max.y, max.z, 1.0f),
        float4(max.x, min.y, max.z, 1.0f),
        float4(max.x, max.y, max.z, 1.0f),
    };

    for(uint i = 0;
            i < 8 && !result;
            ++i)
    {
        // homogeneous p
        float4 hp = (*proj_view) * vertices[i];

        // We are using projection matrix which puts z to 0 to 1
        if((hp.x >= -hp.w && hp.x <= hp.w) &&
            (hp.y >= -hp.w && hp.y <= hp.w) &&
            (hp.z >= 0 && hp.z <= hp.w))
        {
            result = true;
            break;
        }
    }

    return result;
}

[[object]]
void grass_object_function(object_data Payload *payloadOutput [[payload]],
                          const device GrassObjectFunctionInput *object_function_input [[buffer (0)]],
                          const device uint *hashes [[buffer (1)]],
                          const device float *floor_z_values [[buffer (2)]],
                          const device float *time_elasped_from_start [[buffer (3)]],
                          constant float4x4 *proj_view [[buffer (4)]],
                          const device float *perlin_noise_values [[buffer (5)]],
                          uint thread_index [[thread_index_in_threadgroup]], // thread index in object threadgroup 
                          uint2 thread_count_per_threadgroup [[threads_per_threadgroup]],
                          uint2 thread_count_per_grid [[threads_per_grid]],
                          uint2 thread_position_in_grid [[thread_position_in_grid]], 
                          uint2 threadgroup_position_in_grid [[threadgroup_position_in_grid]], 
                          mesh_grid_properties mgp)
{
    // NOTE(gh) These cannot be thread_index, because the buffer we are passing has all the data for the 'grid' 
    uint hash = hashes[thread_position_in_grid.y * thread_count_per_grid.x + thread_position_in_grid.x];

    // Frustum cull
    float random01 = random_between_0_1(hash, 0, 0);
    float length = 2.8f + random01;
    float3 pad = float3(length, length, 0.0f);

    float2 threadgroup_dim = float2(thread_count_per_threadgroup.x * object_function_input->one_thread_worth_dim.x, 
                                    thread_count_per_threadgroup.y * object_function_input->one_thread_worth_dim.y);

    float3 min = float3(object_function_input->min + 
                          float2(threadgroup_position_in_grid.x * threadgroup_dim.x, threadgroup_position_in_grid.y * threadgroup_dim.y),
                          0.0f);
    // TODO(gh) Also need to think carefully about the z value
    float3 max = min + float3(threadgroup_dim,
                        10.0f);

    // TODO(gh) Should watch out for diverging, but because the min & max value does not differ per thread,
    // I think this will be fine.
    if(is_inside_frustum(proj_view, min - pad, max + pad))
    {
        float z = floor_z_values[thread_position_in_grid.y * thread_count_per_grid.x + thread_position_in_grid.x];

        float center_x = object_function_input->min.x + object_function_input->one_thread_worth_dim.x * ((float)thread_position_in_grid.x + 0.5f);
        float center_y = object_function_input->min.y + object_function_input->one_thread_worth_dim.y * ((float)thread_position_in_grid.y + 0.5f);

        payloadOutput->per_grass_data[thread_index].center = packed_float3(center_x, center_y, z);
        payloadOutput->per_grass_data[thread_index].blade_width = 0.135f;
        payloadOutput->per_grass_data[thread_index].length = length;

        float noise = perlin_noise_values[thread_position_in_grid.y * thread_count_per_grid.x + thread_position_in_grid.x];
        payloadOutput->per_grass_data[thread_index].tilt = clamp(1.9f + 0.7f*random01 + noise, 0.0f, length - 0.01f);

        // payloadOutput->per_grass_data[thread_index].facing_direction = packed_float2(cos(0.0f), sin(0.0f));
        payloadOutput->per_grass_data[thread_index].facing_direction = packed_float2(cos((float)hash), sin((float)hash));
        payloadOutput->per_grass_data[thread_index].bend = 0.8f;
        payloadOutput->per_grass_data[thread_index].wiggliness = 2.0f + 1.0f*(random01);
        payloadOutput->per_grass_data[thread_index].time_elasped_from_start = *time_elasped_from_start;
        payloadOutput->per_grass_data[thread_index].color = packed_float3(random01, 0.784f, 0.2f);

        // Hashes got all the values for the grid
        payloadOutput->per_grass_data[thread_index].hash = hash;

        if(thread_index == 0)
        {
            // TODO(gh) How does GPU know when it should fire up the mesh grid? 
            // Does it do it when all threads in object threadgroup finishes?
            // NOTE(gh) Each object thread _should_ spawn one mesh threadgroup
            mgp.set_threadgroups_per_grid(uint3(object_thread_per_threadgroup_count_x, object_thread_per_threadgroup_count_y, 1));
        }
    }
}

// NOTE(gh) simplifed form of (1-t)*{(1-t)*p0+t*p1} + t*{(1-t)*p1+t*p2}
float3
quadratic_bezier(float3 p0, float3 p1, float3 p2, float t)
{
    float one_minus_t = 1-t;

    return one_minus_t*one_minus_t*p0 + 2*t*one_minus_t*p1 + t*t*p2;
}

// NOTE(gh) first derivative = 2*(1-t)*(p1-p0) + 2*t*(p2-p1)
float3
quadratic_bezier_first_derivative(float3 p0, float3 p1, float3 p2, float t)
{
    float3 result = 2*(1-t)*(p1-p0) + 2*t*(p2-p1);

    return result;
}

GBufferVertexOutput
calculate_grass_vertex(const object_data PerGrassData *per_grass_data, 
                        uint thread_index, 
                        constant float4x4 *proj_view,
                        constant float4x4 *light_proj_view,
                        constant packed_float3 *camera_p,
                        uint grass_vertex_count,
                        uint grass_divide_count)
{
    packed_float3 center = per_grass_data->center;
    float blade_width = per_grass_data->blade_width;
    float length = per_grass_data->length; // length of the blade
    float tilt = per_grass_data->tilt; // z value of the tip
    float stride = sqrt(length*length - tilt*tilt); // only horizontal length of the blade
    packed_float2 facing_direction = normalize(per_grass_data->facing_direction);
    float bend = per_grass_data->bend;
    float wiggliness = per_grass_data->wiggliness;
    uint hash = per_grass_data->hash;
    float time_elasped_from_start = per_grass_data->time_elasped_from_start;

    float3 p0 = center;

    float3 p2 = p0 + stride * float3(facing_direction, 0.0f) + float3(0, 0, tilt);  
    float3 orthogonal_normal = normalize(float3(-facing_direction.y, facing_direction.x, 0.0f)); // Direction of the width of the grass blade, think it should be (y, -x)?

    float3 blade_normal = normalize(cross(p2 - p0, orthogonal_normal)); // normal of the p0 and p2, will be used to get p1 
    
    // TODO(gh) bend value is a bit unintuitive, because this is not represented in world unit.
    // But if bend value is 0, it means the grass will be completely flat
    float3 p1 = p0 + (2.5f/4.0f) * (p2 - p0) + bend * blade_normal;

    float t = (float)(thread_index / 2) / (float)grass_divide_count;
    float hash_value = hash*pi_32;
    float wind_factor = t * wiggliness + hash_value + time_elasped_from_start;

    // float3 modified_p2 = p2 + 0.18f * sin(wind_factor) * (-blade_normal);
    float3 modified_p2 = p2 + float3(0, 0, 0.18f * sin(wind_factor));
    float3 modified_p1 = p1 + float3(0, 0, 0.15f * sin(wind_factor));
    // float3 modified_p2 = p2 + float3(0, 0, 0.15f * sin(wind_factor));

    float3 world_p = quadratic_bezier(p0, modified_p1, modified_p2, t);

    if(thread_index == grass_vertex_count-1)
    {
        world_p += 0.5f * blade_width * orthogonal_normal;
    }
    else
    {
        // TODO(gh) Clean this up! 
        // TODO(gh) Original method do it in view space, any reason to do that
        // (grass possibly facing the direction other than z)?
        bool should_shift_thread_mod_1 = (dot(orthogonal_normal, *camera_p - center) < 0);
        float shift = 0.08f;
        if(thread_index%2 == 1)
        {
            world_p += blade_width * orthogonal_normal;
            if(should_shift_thread_mod_1 && thread_index != 1)
            {
                world_p.z += shift;
            }
        }
        else
        {
            if(!should_shift_thread_mod_1 && thread_index != 0)
            {
                world_p.z += shift;
            }
        }
    }


    GBufferVertexOutput result;
    result.clip_p = (*proj_view) * float4(world_p, 1.0f);
    result.p = world_p;
    result.N = normalize(cross(quadratic_bezier_first_derivative(p0, modified_p1, modified_p2, t), orthogonal_normal));
    result.color = per_grass_data->color;
    result.depth = result.clip_p.z / result.clip_p.w;
    float4 p_in_light_coordinate = (*light_proj_view) * float4(world_p, 1.0f);
    result.p_in_light_coordinate = p_in_light_coordinate.xyz / p_in_light_coordinate.w;

    return result;
}

// NOTE(gh) not being used, but mesh shader requires us to provide a struct for per-primitive data
struct StubPerPrimitiveData
{
};

using SingleGrassTriangleMesh = metal::mesh<GBufferVertexOutput, // per vertex 
                                            StubPerPrimitiveData, // per primitive
                                            grass_high_lod_vertex_count, // max vertex count 
                                            grass_high_lod_triangle_count, // max primitive count
                                            metal::topology::triangle>;

// For the grass, we should launch at least triange count * 3 threads per one mesh threadgroup(which represents one grass blade),
// because one thread is spawning one index at a time
// TODO(gh) That being said, this is according to the wwdc mesh shader video. 
// Double check how much thread we actually need to fire later!
[[mesh]] 
void single_grass_mesh_function(SingleGrassTriangleMesh output_mesh,
                                const object_data Payload *payload [[payload]],
                                constant float4x4 *game_proj_view [[buffer(0)]],
                                constant float4x4 *render_proj_view [[buffer(1)]],
                                constant float4x4 *light_proj_view [[buffer(2)]],
                                constant packed_float3 *game_camera_p [[buffer(3)]],
                                constant packed_float3 *render_camera_p [[buffer(4)]],
                                uint thread_index [[thread_index_in_threadgroup]],
                                uint2 threadgroup_position [[threadgroup_position_in_grid]],
                                uint2 threadgroup_count_per_grid [[threadgroups_per_grid]])
{
    const object_data PerGrassData *per_grass_data = &(payload->per_grass_data[threadgroup_position.y * threadgroup_count_per_grid.x + threadgroup_position.x]);
    
    float low_lod_distance_square = 3000;
    // float low_lod_distance_square = 300000000;
    // TODO(gh) We can also do this in object shader, per threadgroup
    uint grass_divide_count = grass_high_lod_divide_count;
    uint grass_vertex_count = grass_high_lod_vertex_count;
    uint grass_index_count = grass_high_lod_index_count;
    uint grass_triangle_count = grass_high_lod_triangle_count;
    if(length_squared(*game_camera_p - per_grass_data->center) > low_lod_distance_square)
    {
        grass_divide_count = grass_low_lod_divide_count;
        grass_vertex_count = grass_low_lod_vertex_count;
        grass_index_count = grass_low_lod_index_count;
        grass_triangle_count = grass_low_lod_triangle_count;
    }

    // TODO(gh) Check there is an actual performance gain by doing this, because it seems like
    // when we become too conservative, there isn't much culling going on in per-grass basis anyway.
    // Maybe it becomes important when we have great z difference?
    // if(is_inside_frustum(game_proj_view, min, max))
    {
        // these if statements are needed, as we are firing more threads than the grass vertex count.
        if (thread_index < grass_vertex_count)
        {
            output_mesh.set_vertex(thread_index, calculate_grass_vertex(per_grass_data, thread_index, render_proj_view, light_proj_view, game_camera_p, grass_vertex_count, grass_divide_count));
        }
        if (thread_index < grass_index_count)
        {
            // For now, we are launching the same amount of threads as the grass index count.
            if(grass_divide_count == grass_high_lod_divide_count)
            {
                output_mesh.set_index(thread_index, grass_high_lod_indices[thread_index]);
            }
            else
            {
                output_mesh.set_index(thread_index, grass_low_lod_indices[thread_index]);
            }
        }
        if (thread_index == 0)
        {
            // NOTE(gh) Only this can fire up the rasterizer and fragment shader
            output_mesh.set_primitive_count(grass_triangle_count);
        }
    }
}

struct MACID
{
    // ID0 is smaller than ID1, no matter what axis that it's opertating on
    int ID0;
    int ID1;
};

// TODO(gh) Should double-check these functions
static MACID
get_mac_index_x(int x, int y, int z, packed_int3 cell_count)
{
    MACID result = {};
    result.ID0 = cell_count.y*(cell_count.x+1)*z + (cell_count.x+1)*y + x;
    result.ID1 = result.ID0+1;
    return result;
}

static float
get_mac_center_value_x(device float *x, int cell_x, int cell_y, int cell_z, constant packed_int3 *cell_count)
{
    MACID macID = get_mac_index_x(cell_x, cell_y, cell_z, *cell_count);

    float result = 0.5f*(x[macID.ID0] + x[macID.ID1]);

    return result;
}

static MACID
get_mac_index_y(int x, int y, int z, packed_int3 cell_count)
{
    MACID result = {};
    result.ID0 = cell_count.x*(cell_count.y+1)*z + (cell_count.y+1)*x + y;
    result.ID1 = result.ID0+1;
    return result;
}

static float
get_mac_center_value_y(device float *y, int cell_x, int cell_y, int cell_z, constant packed_int3 *cell_count)
{
    MACID macID = get_mac_index_y(cell_x, cell_y, cell_z, *cell_count);

    float result = 0.5f*(y[macID.ID0] + y[macID.ID1]);

    return result;
}

static MACID
get_mac_index_z(int x, int y, int z, packed_int3 cell_count)
{
    MACID result = {};
    result.ID0 = cell_count.x*(cell_count.z+1)*y + (cell_count.z+1)*x + z;
    result.ID1 = result.ID0+1;
    return result;
}

static float
get_mac_center_value_z(device float *z, int cell_x, int cell_y, int cell_z, constant packed_int3 *cell_count)
{
    MACID macID = get_mac_index_z(cell_x, cell_y, cell_z, *cell_count);

    float result = 0.5f*(z[macID.ID0] + z[macID.ID1]);

    return result;
}

static int
get_mac_index_center(int x, int y, int z, packed_int3 cell_count)
{
    int result = cell_count.x*cell_count.y*z + cell_count.x*y + x;
    return result;
}


static packed_float3
get_mac_bilinear_center_value(device float *x, device float *y, device float *z, packed_float3 p, 
                                constant packed_int3 *cell_count)
{
    packed_float3 result;

    p.x = clamp(p.x, 0.5f, cell_count->x-0.5f);
    p.y = clamp(p.y, 0.5f, cell_count->y-0.5f);
    p.z = clamp(p.z, 0.5f, cell_count->z-0.5f);

    p -= packed_float3(0.5f, 0.5f, 0.5f);

    // TODO(gh) This might produce slightly wrong value?
    int x0 = floor(p.x);
    int x1 = x0 + 1;
    if(x1 == cell_count->x)
    {
        x0--;
        x1--;
    }
    float xf = p.x-x0;

    int y0 = floor(p.y);
    int y1 = y0 + 1;
    if(y1 == cell_count->y)
    {
        y0--;
        y1--;
    }
    float yf = p.y-y0;

    int z0 = floor(p.z);
    int z1 = z0 + 1;
    if(z1 == cell_count->z)
    {
        z0--;
        z1--;
    }
    float zf = p.z - z0;

    result.x =
            lerp(
                lerp(lerp(get_mac_center_value_x(x, x0, y0, z0, cell_count), xf, get_mac_center_value_x(x, x1, y0, z0, cell_count)), 
                    yf, 
                    lerp(get_mac_center_value_x(x, x0, y1, z0, cell_count), xf, get_mac_center_value_x(x, x1, y1, z0, cell_count))),
                zf,
                lerp(lerp(get_mac_center_value_x(x, x0, y0, z1, cell_count), xf, get_mac_center_value_x(x, x1, y0, z1, cell_count)), 
                    yf, 
                    lerp(get_mac_center_value_x(x, x0, y1, z1, cell_count), xf, get_mac_center_value_x(x, x1, y1, z1, cell_count))));

    result.y = 
            lerp(
                lerp(lerp(get_mac_center_value_y(y, x0, y0, z0, cell_count), xf, get_mac_center_value_y(y, x1, y0, z0, cell_count)), 
                    yf, 
                    lerp(get_mac_center_value_y(y, x0, y1, z0, cell_count), xf, get_mac_center_value_y(y, x1, y1, z0, cell_count))),
                zf,
                lerp(lerp(get_mac_center_value_y(y, x0, y0, z1, cell_count), xf, get_mac_center_value_y(y, x1, y0, z1, cell_count)), 
                    yf, 
                    lerp(get_mac_center_value_y(y, x0, y1, z1, cell_count), xf, get_mac_center_value_y(y, x1, y1, z1, cell_count))));

    result.z = 
            lerp(
                lerp(lerp(get_mac_center_value_z(z, x0, y0, z0, cell_count), xf, get_mac_center_value_z(z, x1, y0, z0, cell_count)), 
                    yf, 
                    lerp(get_mac_center_value_z(z, x0, y1, z0, cell_count), xf, get_mac_center_value_z(z, x1, y1, z0, cell_count))),
                zf,
                lerp(lerp(get_mac_center_value_z(z, x0, y0, z1, cell_count), xf, get_mac_center_value_z(z, x1, y0, z1, cell_count)), 
                    yf, 
                    lerp(get_mac_center_value_z(z, x0, y1, z1, cell_count), xf, get_mac_center_value_z(z, x1, y1, z1, cell_count))));
    return result;
}

#define target_seconds_per_frame (1/60.0f)

// NOTE(gh) p0 is on the ground and does not move
static void
offset_control_points_with_dynamic_wind(device packed_float3 *p0, device packed_float3 *p1, device packed_float3 *p2, 
                                        float original_p0_p1_length, float original_p1_p2_length,
                                         packed_float3 wind, float dt, float noise)
{
    // TODO(gh) We can re-adjust p2 after adjusting p1
    packed_float3 p1_p2 = normalize(*p2 + dt*noise*wind - *p1);
    *p2 = *p1 + original_p1_p2_length*p1_p2;

    packed_float3 p0_p1 = normalize(*p1 + dt*noise*wind - *p0);
    *p1 = *p0 + original_p0_p1_length*p0_p1;
}

static void
offset_control_points_with_spring(thread packed_float3 *original_p1, thread packed_float3 *original_p2,
                                   device packed_float3 *p1, device packed_float3 *p2, float spring_c, float noise, float dt)
{
    float p2_spring_c = spring_c/3.f;

    // NOTE(gh) Reversing the wind noise to improve grass bobbing
    float one_minus_noise = 1 - noise - 0.2f;
    *p1 += dt*one_minus_noise*spring_c*(*original_p1 - *p1);
    *p2 += dt*one_minus_noise*p2_spring_c*(*original_p2 - *p2);
}

kernel void
initialize_grass_grid(device GrassInstanceData *grass_instance_buffer [[buffer(0)]],
                                const device float *floor_z_values [[buffer (1)]],
                                constant GridInfo *grid_info [[buffer (2)]],
                                constant packed_float3 *wind_noise_texture_world_dim [[buffer (3)]],
                                uint2 thread_count_per_grid [[threads_per_grid]],
                                uint2 thread_position_in_grid [[thread_position_in_grid]])
{
    uint grass_index = thread_count_per_grid.x*thread_position_in_grid.y + thread_position_in_grid.x;
    float z = floor_z_values[grass_index];
    float3 p0 = packed_float3(grid_info->min, 0) + 
                    packed_float3(grid_info->one_thread_worth_dim.x*thread_position_in_grid.x, 
                                  grid_info->one_thread_worth_dim.y*thread_position_in_grid.y, 
                                  z);
    
    uint hash = 10000*(wang_hash(grass_index)/(float)0xffffffff);
    float random01 = (float)hash/(float)(10000);
    float length = 2.8h + random01;
    float tilt = clamp(1.9f + 0.7f*random01, 0.0f, length - 0.01f);

    float2 facing_direction = float2(cos((float)hash), sin((float)hash));
    float stride = sqrt(length*length - tilt*tilt); // only horizontal length of the blade
    float bend = 0.6f + 0.2f*random01;

    float3 original_p2 = p0 + stride * float3(facing_direction, 0.0f) + float3(0, 0, tilt);  
    float3 orthogonal_normal = normalize(float3(-facing_direction.y, facing_direction.x, 0.0f)); // Direction of the width of the grass blade, think it should be (y, -x)?
    float3 blade_normal = normalize(cross(original_p2 - p0, orthogonal_normal)); // normal of the p0 and p2, will be used to get p1 
    float3 original_p1 = p0 + (2.0f/4.0f) * (original_p2 - p0) + bend * blade_normal;

    grass_instance_buffer[grass_index].p0 = packed_float3(p0);
    grass_instance_buffer[grass_index].p1 = packed_float3(original_p1);
    grass_instance_buffer[grass_index].p2 = packed_float3(original_p2);

    grass_instance_buffer[grass_index].orthogonal_normal = packed_float3(orthogonal_normal);
    grass_instance_buffer[grass_index].hash = hash; 
    grass_instance_buffer[grass_index].blade_width = 0.125f;
    grass_instance_buffer[grass_index].spring_c = 8.5f + 3*random01;
    grass_instance_buffer[grass_index].color = packed_float3(random01, 0.784h, 0.2h);
    grass_instance_buffer[grass_index].texture_p = p0;
}

kernel void
fill_grass_instance_data_compute(device atomic_uint *grass_count [[buffer(0)]],
                                device GrassInstanceData *grass_instance_buffer [[buffer(1)]],
                                const device float *perlin_noise_values [[buffer (2)]],
                                const device GridInfo *grid_info [[buffer (3)]],
                                constant float4x4 *game_proj_view [[buffer (4)]],
                                device float *fluid_cube_v_x [[buffer (5)]],
                                device float *fluid_cube_v_y [[buffer (6)]],
                                device float *fluid_cube_v_z [[buffer (7)]],
                                constant packed_float3 *fluid_cube_min [[buffer (8)]],
                                constant packed_float3 *fluid_cube_max [[buffer (9)]],
                                constant packed_int3 *fluid_cube_cell_count [[buffer (10)]],
                                constant float *fluid_cube_cell_dim [[buffer (11)]],
                                texture3d<float> wind_noise_texture [[texture(0)]],
                                uint2 thread_count_per_grid [[threads_per_grid]],
                                uint2 thread_position_in_grid [[thread_position_in_grid]])
{
    uint grass_index = thread_count_per_grid.x*thread_position_in_grid.y + thread_position_in_grid.x;
    packed_float3 p0 = grass_instance_buffer[grass_index].p0;
    
    // TODO(gh) better hash function for each grass?
    uint hash = 10000*(wang_hash(grass_index)/(float)0xffffffff);
    float random01 = (float)hash/(float)(10000);
    float grass_length = 2.8h + random01;
    float tilt = clamp(1.9f + 0.7f*random01, 0.0f, grass_length - 0.01f);

#if 0
    // TODO(gh) This does not take account of side curve of the plane, tilt ... so many things
    // Also, we can make the length smaller based on the facing direction
    // These pad values are not well thought out, just throwing those in
    float3 length_pad = 0.6f*float3(length, length, 0.0f);
    float3 min = p0 - length_pad;
    float3 max = p0 + length_pad;
    max.z += tilt + 1.0f;
     
    // TODO(gh) For now, we should disable this and rely on grid based frustum culling
    // because we need to know the previous instance data in certain position
    // which means the instance buffer cannot be mixed up. The solution for this would be some sort of hash table,
    // but we should measure them and see which way would be faster.
    if(is_inside_frustum(game_proj_view, min, max))
#endif
    {
        atomic_fetch_add_explicit(grass_count, 1, memory_order_relaxed);

        float2 facing_direction = float2(cos((float)hash), sin((float)hash));
        float stride = sqrt(grass_length*grass_length - tilt*tilt); // only horizontal length of the blade
        float bend = 0.7f + 0.2f*random01;

        packed_float3 original_p2 = p0 + stride * float3(facing_direction, 0.0f) + float3(0, 0, tilt);  
        packed_float3 orthogonal_normal = normalize(float3(-facing_direction.y, facing_direction.x, 0.0f)); // Direction of the width of the grass blade, think it should be (y, -x)?
        packed_float3 blade_normal = normalize(cross(original_p2 - p0, orthogonal_normal)); // normal of the p0 and p2, will be used to get p1 
        packed_float3 original_p1 = p0 + (2.5f/4.0f) * (original_p2 - p0) + bend * blade_normal;

        float original_p0_p1_length = length(original_p1 - p0);
        float original_p1_p2_length = length(original_p2 - original_p1);

        packed_float3 wind_v = packed_float3(5, 0, 0);
        if(is_inside_cube(p0, *fluid_cube_min, *fluid_cube_max))
        {
            packed_float3 cell_p = (p0 - *fluid_cube_min) / *fluid_cube_cell_dim;

#if 0
            wind_v += get_mac_bilinear_center_value(fluid_cube_v_x, fluid_cube_v_y, fluid_cube_v_z, cell_p, fluid_cube_cell_count);
#else
            int xi = floor(cell_p.x);
            int yi = floor(cell_p.y);
            int zi = floor(cell_p.z);
            wind_v += packed_float3(get_mac_center_value_x(fluid_cube_v_x, xi, yi, zi, fluid_cube_cell_count),
                    get_mac_center_value_y(fluid_cube_v_y, xi, yi, zi, fluid_cube_cell_count),
                    get_mac_center_value_z(fluid_cube_v_z, xi, yi, zi, fluid_cube_cell_count));
#endif
        }

        constexpr sampler s = sampler(coord::normalized, address::repeat, filter::linear);

        // TODO(gh) Should make this right! I guess this is the 'scale' that god of war used?
        float3 texcoord = grass_instance_buffer[grass_index].texture_p/64;
        float wind_noise = wind_noise_texture.sample(s, texcoord).x;
        offset_control_points_with_dynamic_wind(&grass_instance_buffer[grass_index].p0, 
                                                    &grass_instance_buffer[grass_index].p1,
                                                    &grass_instance_buffer[grass_index].p2, 
                                                    original_p0_p1_length, original_p1_p2_length,
                                                    wind_v, target_seconds_per_frame, 
                                                    wind_noise);

#if 1
        offset_control_points_with_spring(&original_p1, &original_p2,
                                            &grass_instance_buffer[grass_index].p1,
                                            &grass_instance_buffer[grass_index].p2, grass_instance_buffer[grass_index].spring_c, wind_noise, target_seconds_per_frame);
#endif

        grass_instance_buffer[grass_index].texture_p -= target_seconds_per_frame*wind_v;
    }
}

struct Arguments 
{
    // TODO(gh) not sure what this is.. but it needs to match with the index of newArgumentEncoderWithBufferIndex
    command_buffer cmd_buffer [[id(0)]]; 
};

// TODO(gh) Use two different vertex functions to support distance-based LOD
kernel void 
encode_instanced_grass_render_commands(device Arguments *arguments[[buffer(0)]],
                                            const device uint *grass_count [[buffer(1)]],
                                            const device GrassInstanceData *grass_instance_buffer [[buffer(2)]],
                                            const device uint *indices [[buffer(3)]],
                                            constant float4x4 *render_proj_view [[buffer(4)]],
                                            constant float4x4 *light_proj_view [[buffer(5)]],
                                            constant packed_float3 *game_camera_p [[buffer(6)]])
{
    render_command command(arguments->cmd_buffer, 0);

    command.set_vertex_buffer(grass_instance_buffer, 0);
    command.set_vertex_buffer(render_proj_view, 1);
    command.set_vertex_buffer(light_proj_view, 2);
    command.set_vertex_buffer(game_camera_p, 3);

    command.draw_indexed_primitives(primitive_type::triangle, // primitive type
                                    39, // index count TODO(gh) We can also just pass those in, too?
                                    indices, // index buffer
                                    *grass_count, // instance count
                                    0, // base vertex
                                    0); //base instance

}

GBufferVertexOutput
calculate_grass_vertex(const device GrassInstanceData *grass_instance_data, 
                        uint thread_index, 
                        constant float4x4 *proj_view,
                        constant float4x4 *light_proj_view,
                        constant packed_float3 *camera_p,
                        uint grass_vertex_count,
                        uint grass_divide_count)
{
    half blade_width = grass_instance_data->blade_width;

    const packed_float3 p0 = grass_instance_data->p0;
    const packed_float3 p1 = grass_instance_data->p1;
    const packed_float3 p2 = grass_instance_data->p2;
    const packed_float3 orthogonal_normal = grass_instance_data->orthogonal_normal;

    float t = (float)(thread_index / 2) / (float)grass_divide_count;

    float3 world_p = quadratic_bezier(p0, p1, p2, t);

    if(thread_index == grass_vertex_count-1)
    {
        world_p += 0.5f * blade_width * orthogonal_normal;
    }
    else
    {
        // TODO(gh) Clean this up! 
        // TODO(gh) Original method do it in view angle, any reason to do that
        // (grass possibly facing the direction other than z)?
        bool should_shift_thread_mod_1 = (dot(orthogonal_normal, *camera_p - p0) < 0);
        float shift = 0.08f;
        if(thread_index%2 == 1)
        {
            world_p += blade_width * orthogonal_normal;
            if(should_shift_thread_mod_1 && thread_index != 1)
            {
                world_p.z += shift;
            }
        }
        else
        {
            if(!should_shift_thread_mod_1 && thread_index != 0)
            {
                world_p.z += shift;
            }
        }
    }


    GBufferVertexOutput result;
    result.clip_p = (*proj_view) * float4(world_p, 1.0f);
    result.p = world_p;
    result.N = normalize(cross(quadratic_bezier_first_derivative(p0, p1, p2, t), orthogonal_normal));
    // TODO(gh) Also make this as a half?
    result.color = packed_float3(grass_instance_data->color);
    result.depth = result.clip_p.z / result.clip_p.w;
    float4 p_in_light_coordinate = (*light_proj_view) * float4(world_p, 1.0f);
    result.p_in_light_coordinate = p_in_light_coordinate.xyz / p_in_light_coordinate.w;

    return result;
}

vertex GBufferVertexOutput
instanced_grass_render_vertex(uint vertexID [[vertex_id]],
                            uint instanceID [[instance_id]],
                            const device GrassInstanceData *grass_instance_buffer [[buffer(0)]],
                            constant float4x4 *render_proj_view [[buffer(1)]],
                            constant float4x4 *light_proj_view [[buffer(2)]],
                            constant packed_float3 *game_camera_p [[buffer(3)]])
                                            
{
    GBufferVertexOutput result = calculate_grass_vertex(grass_instance_buffer + instanceID, 
                                                        vertexID, 
                                                        render_proj_view,
                                                        light_proj_view,
                                                        game_camera_p,
                                                        grass_high_lod_vertex_count,
                                                        grass_high_lod_divide_count);
    
    return result;
}

kernel void
initialize_grass_counts(device atomic_uint *grass_count [[buffer(0)]])
{
    atomic_exchange_explicit(grass_count, 0, memory_order_relaxed);
}


