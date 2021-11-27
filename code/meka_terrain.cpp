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
make_mountain_inside_terrain(loaded_raw_mesh *terrain, 
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

