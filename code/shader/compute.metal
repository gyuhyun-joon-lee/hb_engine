
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
    result[threads_per_grid.x * index.y + index.x] = random_between_0_1(index.x, index.y, 1);
}

