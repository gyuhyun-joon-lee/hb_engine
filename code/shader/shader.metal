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
    // NOTE(gh) This will automatically output to 0th color attachment, which is drawableTexture
    return float4(vertex_output.color, 1.0f);
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
perlin_noise01(float x, float y, float z)
{
    uint xi = floor(x);
    uint yi = floor(y);
    uint zi = floor(z);
    uint x255 = xi % 255; // used for hashing from the random numbers
    uint y255 = yi % 255; // used for hashing from the random numbers
    uint z255 = zi % 255;

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

    uint random_value0 = permutations255[permutations255[permutations255[x255]+y255]+z255];
    uint random_value1 = permutations255[permutations255[permutations255[x255+1]+y255]+z255];
    uint random_value2 = permutations255[permutations255[permutations255[x255]+y255+1]+z255];
    uint random_value3 = permutations255[permutations255[permutations255[x255+1]+y255+1]+z255];

    uint random_value4 = permutations255[permutations255[permutations255[x255]+y255]+z255+1];
    uint random_value5 = permutations255[permutations255[permutations255[x255+1]+y255]+z255+1];
    uint random_value6 = permutations255[permutations255[permutations255[x255]+y255+1]+z255+1];
    uint random_value7 = permutations255[permutations255[permutations255[x255+1]+y255+1]+z255+1];

    // NOTE(gh) -1 are for getting the distance vector
    float gradient0 = get_gradient_value(random_value0, xf, yf, zf);
    float gradient1 = get_gradient_value(random_value1, xf-1, yf, zf);
    float gradient2 = get_gradient_value(random_value2, xf, yf-1, zf);
    float gradient3 = get_gradient_value(random_value3, xf-1, yf-1, zf);

    float gradient4 = get_gradient_value(random_value4, xf, yf, zf-1);
    float gradient5 = get_gradient_value(random_value5, xf-1, yf, zf-1);
    float gradient6 = get_gradient_value(random_value6, xf, yf-1, zf-1);
    float gradient7 = get_gradient_value(random_value7, xf-1, yf-1, zf-1);

    // NOTE(gh) mix == lerp
    float lerp01 = mix(gradient0, gradient1, u); // lerp between 0 and 1
    float lerp23 = mix(gradient2, gradient3, u); // lerp between 2 and 3 
    float lerp0123 = mix(lerp01, lerp23, v); // lerp between '0-1' and '2-3', in y direction

    float lerp45 = mix(gradient4, gradient5, u); // lerp between 4 and 5
    float lerp67 = mix(gradient6, gradient7, u); // lerp between 6 and 7 
    float lerp4567 = mix(lerp45, lerp67, v); // lerp between '4-5' and '6-7', in y direction

    float lerp01234567 = mix(lerp0123, lerp4567, w);

    float result = (lerp01234567 + 1.0f) * 0.5f; // put into 0 to 1 range

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
    float perlin_noise_value = perlin_noise01(x, y, 0.0f);

    float4 result = float4(perlin_noise_value, perlin_noise_value, perlin_noise_value, 0.2f);

    return result;
}

struct ShowPerlinNoiseGridVertexOutput
{
    float4 clip_p [[position]];
    float3 color;
};

vertex ShowPerlinNoiseGridVertexOutput
forward_show_perlin_noise_grid_vert(uint vertexID [[vertex_id]],
                            uint instanceID [[instance_id]],
                             constant float *vertices [[buffer(0)]], 
                             constant float2 *grid_dim [[buffer(1)]],
                             constant float *perlin_values [[buffer(2)]],
                             constant float *floor_z_values [[buffer(3)]],
                             constant uint2 *grass_count [[buffer(4)]],
                             constant PerFrameData *per_frame_data [[buffer(5)]])
{
    ShowPerlinNoiseGridVertexOutput result = {};

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
forward_show_perlin_noise_grid_frag(ShowPerlinNoiseGridVertexOutput vertex_output [[stage_in]])
{
    float4 result = float4(vertex_output.color, 0.7f);
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
    float random01 = random_between_0_1(hash, 0, 0);

    // Frustum cull
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
        payloadOutput->per_grass_data[thread_index].blade_width = 0.195f;
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
        // TODO(gh) Original method do it in view angle, any reason to do that
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

#if 1
    // TODO(gh) This does not take account of side curve of the plane, tilt ... so many things
    // Also, we can make the length smaller based on the facing direction
    // These pad values are not well thought out, just throwing those in
    float3 length = 0.6f*float3(per_grass_data->length, per_grass_data->length, 0.0f);
    float3 min = per_grass_data->center - length;
    float3 max = per_grass_data->center + length;
    max.z += per_grass_data->tilt + 1.0f;
#endif
    
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

struct ShowFrustumVetexOutput
{
    float4 clip_p [[position]];
};

vertex ShowFrustumVetexOutput
forward_show_frustum_vert(uint vertexID [[vertex_id]], 
                  constant packed_float3 *vertices [[buffer(0)]],
                  constant float4x4 *proj_view [[buffer(1)]])
{
    ShowFrustumVetexOutput result = {};
    result.clip_p = (*proj_view) * float4(vertices[vertexID], 1.0f);

    return result;
}

fragment float4
forward_show_frustum_frag(ShowFrustumVetexOutput vertex_output [[stage_in]])
{
    float4 result = float4(0, 0, 1, 0.2f);
    return result;
}

kernel void
fill_grass_instance_data_compute(device atomic_uint *grass_count [[buffer(0)]],
                                device GrassInstanceData *grass_instance_buffer [[buffer(1)]],
                                uint2 thread_count_per_grid [[threads_per_grid]],
                                uint2 thread_position_in_grid [[thread_position_in_grid]])
{
    // TODO(gh) should check if this is really 'atomic'
    uint grass_index = atomic_fetch_add_explicit(grass_count, 1, memory_order_relaxed);

    grass_instance_buffer[grass_index].center = 1.0f*packed_float3(thread_position_in_grid.x, thread_position_in_grid.y, 0);
    grass_instance_buffer[grass_index].hash = grass_index; 
    grass_instance_buffer[grass_index].blade_width = 1;
    grass_instance_buffer[grass_index].length = 1;
    grass_instance_buffer[grass_index].tilt = 1;
    grass_instance_buffer[grass_index].facing_direction = packed_float2(1, 0);
    grass_instance_buffer[grass_index].bend = 1;
    grass_instance_buffer[grass_index].wiggliness = 1;
    grass_instance_buffer[grass_index].color = packed_float3(1, 0, 0);
    grass_instance_buffer[grass_index].time_elasped_from_start = 0; // TODO(gh) Do we even need to pass this?
    grass_instance_buffer[grass_index].pad = 1;
}

struct Arguments 
{
    // TODO(gh) not sure what this is.. but it needs to match with the index of newArgumentEncoderWithBufferIndex
    command_buffer cmd_buffer [[id(0)]]; 
};

kernel void encode_instanced_grass_render_commands(device Arguments *arguments[[buffer(0)]],
                                            device uint *grass_count [[buffer(1)]],
                                            device GrassInstanceData *grass_instance_buffer [[buffer(2)]],
                                            device uint *indices [[buffer(3)]]) 
{
    render_command command(arguments->cmd_buffer, 0);

    command.set_vertex_buffer(grass_instance_buffer, 0);

    command.draw_indexed_primitives(primitive_type::triangle, // primitive type
                                    39, // index count TODO(gh) We can also just pass those in, too?
                                    indices, // index buffer
                                    *grass_count, // instance count
                                    0, // base vertex
                                    0); //base instance
}

GBufferVertexOutput
calculate_grass_vertex(float3 center, 
                        uint thread_index, 
                        constant float4x4 *proj_view,
                        constant float4x4 *light_proj_view,
                        constant packed_float3 *camera_p,
                        uint grass_vertex_count,
                        uint grass_divide_count,
                        float time_elasped)
{
    float blade_width = 0.2f;
    float length = 3.0f; // length of the blade
    float tilt = 2.0f; // z value of the tip
    float stride = sqrt(length*length - tilt*tilt); // only horizontal length of the blade
    packed_float2 facing_direction = packed_float2(1, 0);
    float bend = 0.5f;
    float wiggliness = 2.1f;
    uint hash = 2;

    float3 p0 = center;

    float3 p2 = p0 + stride * float3(facing_direction, 0.0f) + float3(0, 0, tilt);  
    float3 orthogonal_normal = normalize(float3(-facing_direction.y, facing_direction.x, 0.0f)); // Direction of the width of the grass blade, think it should be (y, -x)?

    float3 blade_normal = normalize(cross(p2 - p0, orthogonal_normal)); // normal of the p0 and p2, will be used to get p1 
    
    // TODO(gh) bend value is a bit unintuitive, because this is not represented in world unit.
    // But if bend value is 0, it means the grass will be completely flat
    float3 p1 = p0 + (2.5f/4.0f) * (p2 - p0) + bend * blade_normal;

    float t = (float)(thread_index / 2) / (float)grass_divide_count;
    float hash_value = hash*pi_32;
    float wind_factor = t * wiggliness + hash_value + time_elasped;

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
        // TODO(gh) Original method do it in view angle, any reason to do that
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
    result.color = float3(1, 0, 0);
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
                            constant packed_float3 *game_camera_p [[buffer(3)]],
                            constant float *time_elasped [[buffer(4)]])
                                            
{
    GBufferVertexOutput result = calculate_grass_vertex(grass_instance_buffer[instanceID].center, 
                                                        vertexID, 
                                                        render_proj_view,
                                                        light_proj_view,
                                                        game_camera_p,
                                                        grass_low_lod_vertex_count,
                                                        grass_low_lod_divide_count,
                                                        *time_elasped);
    
    return result;
}




