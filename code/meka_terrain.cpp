inline i32
round_r32_i32(r32 value)
{
    // TODO(joon) : intrinsic?
    return (i32)roundf(value);
}

// TODO(joon) : maket this to use random table
internal r32
random_between_0_1()
{
    r32 result = (r32)rand()/(r32)RAND_MAX;
    return result;
}

internal r32
random_between(r32 min, r32 max)
{
    return min + (max-min)*random_between_0_1();
}

internal r32
random_between_minus_1_1()
{
    r32 result = 2.0f*((r32)rand()/(r32)RAND_MAX) - 1.0f;
    return result;
}

inline r32
random_range(r32 value, r32 range)
{
    r32 half_range = range/2.0f;

    r32 result = value + half_range*random_between_minus_1_1();

    return result;
}

internal void
generate_mountain_inside_terrain(raw_mesh *terrain, 
                            i32 x_count, i32 z_count,
                            r32 center_x, r32 center_z, 
                            i32 stride,
                            r32 dim, r32 radius, r32 max_height)
{
    i32 min_x = maximum(round_r32_i32((center_x - radius)/dim), 0);
    i32 max_x = minimum(round_r32_i32((center_x + radius)/dim), x_count);

    i32 min_z = maximum(round_r32_i32((center_z - radius)/dim), 0);
    i32 max_z = minimum(round_r32_i32((center_z + radius)/dim), z_count);
    
    i32 center_x_i32 = (i32)((max_x + min_x)/2.0f);
    i32 center_z_i32 = (i32)((max_z + min_z)/2.0f);

    v3 *row = terrain->positions + min_z*stride + min_x;
    for(i32 z = min_z;
            z < max_z;
            z++)
    {
        v3 *p = row;

        r32 y = 0.0f;
        for(i32 x = min_x;
            x < max_x;
            x++)
        {
            r32 distance = dim*Length(V2i(center_x_i32, center_z_i32) - V2i(x, z));
            if(distance <= radius)
            {
                r32 height = (1.0f - (distance / radius))*max_height;
                p->y = random_range(height, 0.45f*height);
                //p->y = height*random_between_0_1();
            }

            p++;
        }

        row += stride;
    }
}

/*
   NOTE/Joon: when we generate a terrain, we think it as a plane full of quads, and manipulate
   the y value of it.
   quad_width = how many quad horizontally
   quad_width = how many quad vertically
*/
internal raw_mesh
generate_simple_terrain(memory_arena *memory_arena, u32 quad_width, u32 quad_height)
{
    // 100 vertices means there will be 99 quads per line
    raw_mesh terrain = {};
    terrain.position_count = (quad_width) * (quad_height);
    terrain.positions = push_array(memory_arena, v3, terrain.position_count);

    r32 startingX = 0;
    r32 startingZ = 0;
    r32 dim = 5;
    for(u32 z = 0;
            z < quad_height;
            ++z)
    {
        for(u32 x = 0;
                x < quad_width;
                ++x)
        {
            r32 randomY = random_between_minus_1_1();
            r32 yRange = 2.f;
            v3 p = V3(startingX + x*dim, yRange*randomY, startingZ + z*dim);
            terrain.positions[z*quad_width + x] = p;
        }
    }

    for(u32 mountain_index = 0;
            mountain_index < 12;
            mountain_index++)
    {
        r32 x = random_between(0, quad_width*dim);
        r32 z = random_between(0, quad_height*dim);

        r32 height = random_between(20, 100);
        r32 radius = random_between(10, 100);
        generate_mountain_inside_terrain(&terrain, 
                quad_width, quad_height, 
                x, z, quad_width,
                dim, radius, height);
    }

    terrain.index_count = 2*3*(quad_height - 1)*(quad_width - 1);
    terrain.indices = push_array(memory_arena, u32, terrain.index_count);

    u32 indexIndex = 0;
    for(u32 z = 0;
            z < quad_height-1;
            ++z)
    {
        for(u32 x = 0;
                x < quad_width-1;
                ++x)
        {
            u32 startingIndex = z*quad_height + x;


            terrain.indices[indexIndex++] = (u16)startingIndex;
            terrain.indices[indexIndex++] = (u16)(startingIndex+quad_width);
            terrain.indices[indexIndex++] = (u16)(startingIndex+quad_width+1);

            terrain.indices[indexIndex++] = (u16)startingIndex;
            terrain.indices[indexIndex++] = (u16)(startingIndex+quad_width+1);
            terrain.indices[indexIndex++] = (u16)(startingIndex+1);

            assert(indexIndex <= terrain.index_count);
        }
    }
    assert(indexIndex == terrain.index_count);

    return terrain;
}

