
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

// NOTE(gh) simplifed form of (1-t)*{(1-t)*p0+t*p1} + t*{(1-t)*p1+t*p2}
packed_float3
quadratic_bezier(packed_float3 p0, packed_float3 p1, packed_float3 p2, float t)
{
    float one_minus_t = 1-t;

    return one_minus_t*one_minus_t*p0 + 2*t*one_minus_t*p1 + t*t*p2;
}

struct GrassInfo
{
    packed_float 
};

struct GrassVertex
{
    packed_float3 p[15];
    packed_float3 normal[15];

    float width;
    float height;
};

// TODO(gh) we can also pass the struct... 
kernel void
fill_grass_vertices(device GrassVertex *result)
{
    // TODO(gh) This assumes that the grass divided count is 7;
    uint grass_divided_count;

    v3 orthogonal_normal = normalize(V3(-facing_direction.y, facing_direction.x, 0.0f)); // Direction of the width of the grass blade

    for(uint i = 0;
            i < grass_divided_count; 
            ++i)
    {
        float t = (float)i/(float)grass_divided_count;

        packed_float3 point_on_bezier_curve = quadratic_bezier(p0, p1, p2, t);

        // The first vertex is the point on the bezier curve,
        // and the next vertex is along the line that starts from the first vertex
        // and goes toward the orthogonal normal.
        // TODO(gh) Taper the grass blade down
        result->p[2*i] = point_on_bezier_curve;
        result->normal[2*i+1] = point_on_bezier_curve + result->width * orthogonal_normal;

        v3 normal = normalize(cross(quadratic_bezier_first_derivative(p0, p1, p2, t), orthogonal_normal));
        vertices[2*i].normal = normal;
        vertices[2*i+1].normal = normal;
    }
}


