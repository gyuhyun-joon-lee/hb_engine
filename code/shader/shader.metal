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

// From one the the Apple's sample shader codes - 
// https://developer.apple.com/library/archive/samplecode/MetalShaderShowcase/Listings/MetalShaderShowcase_AAPLWoodShader_metal.html
float random_between_0_1(int x, int y, int z)
{
    int seed = x + y * 57 + z * 241;
    seed = (seed << 13) ^ seed;
    return (( 1.0 - ( (seed * (seed * seed * 15731 + 789221) + 1376312589) & 2147483647) / 1073741824.0f) + 1.0f) / 2.0f;
}

// TODO(gh) Find out what device means exactly(device address == gpu side memory?)
// TODO(gh) Should we also use multiple threadgroups to represent one grid?
kernel void 
get_random_numbers(device float* result,
                            uint2 threads_per_grid[[threads_per_grid]],
                            uint2 index [[thread_position_in_grid]])
{
    uint start = 3 * (threads_per_grid.x * index.y + index.x);

    result[start + 0] =  random_between_0_1(index.x, index.y, 1); // x
    result[start + 1] =  random_between_0_1(index.x, index.y, 1); // y
    result[start + 2] =  1; // will be populated by the CPU
}


struct Payload
{
    // TODO(gh) I would guess that one object thread produces one payload,
    // but if not, this needs to be an array with size of mesh thread group count
    float3 center;
};


[[object]]
void grass_object_function(object_data Payload *payloadOutput [[payload]],
                          const device GrassObjectFunctionInput *object_function_input[[buffer (0)]],
                          uint thread_index [[thread_index_in_threadgroup]], // thread index in object threadgroup 
                          uint2 thread_position [[thread_position_in_grid]], 
                          mesh_grid_properties mgp)
{
    float center_x = object_function_input->one_thread_worth_dim.x * (thread_position.x + 0.5f);
    float center_y = object_function_input->one_thread_worth_dim.y * (thread_position.y + 0.5f);

    // TODO(gh) also feed z value
    payloadOutput[thread_index].center = float3(center_x, center_y, 0.0f);

    // TODO(gh) How does GPU know when it should fire up the mesh grid? 
    // Does it do it when all threads in object threadgroup finishes?
    mgp.set_threadgroups_per_grid(uint3(object_function_input->mesh_threadgroup_count_x, object_function_input->mesh_threadgroup_count_y, 0));
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
calculate_grass_vertex(const object_data Payload *payload, 
                        uint thread_index, 
                        constant float4x4 *proj_view,
                        constant float4x4 *light_proj_view)
{
    // TODO(gh) make grass parameters configurable
    float blade_width = 0.2f;
    float stride = 2.0f;
    float height = 1.8f;
    float2 facing_direction = float2(1, 0);
    float bend = 0.5f;
    float wiggliness = 2.1f;
    float3 color = float3(0.2f, 0.8f, 0.2f);

    float3 p0 = payload->center;

    float3 p2 = p0 + stride * float3(facing_direction, 0.0f) + float3(0, 0, height);  
    float3 orthogonal_normal = normalize(float3(-facing_direction.y, facing_direction.x, 0.0f)); // Direction of the width of the grass blade

    float3 blade_normal = normalize(cross(p2 - p0, orthogonal_normal)); // normal of the p0 and p2, will be used to get p1 
    
    // TODO(gh) bend value is a bit unintuitive, because this is not represented in world unit.
    // But if bend value is 0, it means the grass will be completely flat
    float3 p1 = p0 + (2.5f/4.0f) * (p2 - p0) + bend * blade_normal;

    float t = (float)(thread_index / 2);
    uint hash = 1123;
    float hash_value = hash*pi_32;
    // TODO(gh) how do we get time since engine startup?
    float time = 0.0f;
    // float exponent = 4.0f*(t-1.0f);
    // float wind_factor = 0.5f*power(euler_contant, exponent) * t * wiggliness + hash*tau_32 + time;
    float wind_factor = t * wiggliness + hash_value + time;

    float3 modified_p1 = p1 + float3(0, 0, 0.1f * sin(wind_factor));
    float3 modified_p2 = p2 + float3(0, 0, 0.15f * sin(wind_factor));

    float3 world_p = quadratic_bezier(p0, modified_p1, modified_p2, t);

    if(thread_index%2 == 1)
    {
        world_p += blade_width * orthogonal_normal;
    }

    GBufferVertexOutput result;
    result.clip_p = (*proj_view) * float4(world_p, 1.0f);
    result.p = world_p;
    result.N = normalize(cross(quadratic_bezier_first_derivative(p0, modified_p1, modified_p2, t), orthogonal_normal));
    result.color = color;
    result.depth = result.clip_p.z / result.clip_p.w;
    float4 p_in_light_coordinate = (*light_proj_view) * float4(world_p, 1.0f);
    result.p_in_light_coordinate = p_in_light_coordinate.xyz / p_in_light_coordinate.w;

    return result;
}

// TODO(gh) Can we change this dynamically? I would suspect no...
constant uint grass_vertex_count = 15;
constant uint grass_triangle_count = 13;
constant uint grass_index_count = 39;

struct StubPrimitive
{
};

using SingleGrassTriangleMesh = metal::mesh<GBufferVertexOutput, // per vertex 
                                            StubPrimitive, // per primitive
                                            grass_vertex_count, // max vertex count 
                                            grass_triangle_count, // max primitive count
                                            metal::topology::triangle>;

// For the grass, we should launch at least triange count * 3 threads per one mesh threadgroup(which represents one grass blade),
// because one thread is spawning one index at a time
// TODO(gh) That being said, this is according to the wwdc mesh shader video. Double check how much thread we actually need to fire
// later!
[[mesh]] 
void single_grass_mesh_function(SingleGrassTriangleMesh output_mesh,
                                const object_data Payload *payload [[payload]],
                                constant float4x4 *proj_view[[buffer(0)]],
                                constant float4x4 *light_proj_view[[buffer(1)]],
                                constant uint *indices[[buffer(2)]],
                                uint thread_index [[thread_index_in_threadgroup]])
{
    // these if statements are needed, as we are firing more threads than the grass vertex count.
    if (thread_index < grass_vertex_count)
    {
        output_mesh.set_vertex(thread_index, calculate_grass_vertex(payload, thread_index, proj_view, light_proj_view));
    }
    if (thread_index < grass_index_count)
    {
        // For now, we are launching the same amount of threads as the grass index count.
        output_mesh.set_index(thread_index, indices[thread_index]);
    }
    if (thread_index == 0)
    {
        // NOTE(gh) Only this can fire up the rasterizer and fragment shader
        output_mesh.set_primitive_count(grass_triangle_count);
    }
}




