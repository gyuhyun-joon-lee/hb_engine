internal void
generate_mountain_inside_terrain(RawMesh *terrain, 
                                i32 x_count, i32 y_count,
                                v2 center,
                                i32 stride,
                                r32 dim, r32 radius, r32 max_height)
{
    // TODO(joon): Get rid of rand()
    u32 random_seed = (u32)rand();
    RandomSeries series = start_random_series(random_seed); 

    i32 min_x = maximum(round_r32_to_i32((center.x - radius)/dim), 0);
    i32 max_x = minimum(round_r32_to_i32((center.x + radius)/dim), x_count);

    i32 min_y = maximum(round_r32_to_i32((center.y - radius)/dim), 0);
    i32 max_y = minimum(round_r32_to_i32((center.y + radius)/dim), y_count);
    
    i32 center_x_i32 = (i32)((max_x + min_x)/2.0f);
    i32 center_y_i32 = (i32)((max_y + min_y)/2.0f);

    v3 *row = terrain->positions + min_y*stride + min_x;
    for(i32 y = min_y;
            y < max_y;
            y++)
    {
        v3 *p = row;

        for(i32 x = min_x;
            x < max_x;
            x++)
        {
            r32 distance = dim*length(V2i(center_x_i32, center_y_i32) - V2i(x, y));
            if(distance <= radius)
            {
                r32 height = (1.0f - (distance / radius))*max_height;
                p->z = random_range(&series, height, 0.45f*height);
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
internal RawMesh
generate_plane_terrain_mesh(MemoryArena *memory_arena, u32 quad_width, u32 quad_height)
{
    // TODO(joon): Get rid of rand()
    u32 random_seed = (u32)rand();
    RandomSeries series = start_random_series(random_seed); 

    // 100 vertices means there will be 99 quads per line
    RawMesh terrain = {};
    terrain.position_count = (quad_width) * (quad_height);
    terrain.positions = push_array(memory_arena, v3, terrain.position_count);

    r32 dim = 1;
    r32 startingX = -dim*(quad_width/2);
    r32 startingY = -dim*(quad_height/2);
    for(u32 y = 0;
            y < quad_height;
            ++y)
    {
        for(u32 x = 0;
                x < quad_width;
                ++x)
        {
            r32 random_z = dim*random_between(&series, -0.5f, 0.5f);
            
            v3 p = V3(startingX + x*dim, startingY + y*dim, random_z);
            terrain.positions[y*quad_width + x] = p;
        }
    }

    for(u32 mountain_index = 0;
            mountain_index < 12;
            mountain_index++)
    {
        v2 mountain_center = V2(random_between(&series, 0, quad_width*dim),
                                random_between(&series, 0, quad_height*dim));

        r32 height = dim*random_between(&series, 5, 25);
        r32 radius = dim*random_between(&series, quad_width/6, quad_width/14);
        generate_mountain_inside_terrain(&terrain, 
                quad_width, quad_height, 
                mountain_center, quad_width,
                dim, radius, height);
    }

    terrain.index_count = 2*3*(quad_height - 1)*(quad_width - 1);
    terrain.indices = push_array(memory_arena, u32, terrain.index_count);

    u32 indexIndex = 0;
    for(u32 y = 0;
            y < quad_height-1;
            ++y)
    {
        for(u32 x = 0;
                x < quad_width-1;
                ++x)
        {
            /*
               NOTE/Joon: Given certain cycle, we will construct the mesh like this
               v2-----v3
               |       |
               |       |
               |       |
               v0-----v1 -> indices : 012, 132
            */
            u32 startingIndex = y*quad_height + x;
            u32 i0 = y*quad_height + x;
            u32 i1 = i0 + 1;
            u32 i2 = i0 + quad_width;
            u32 i3 = i2 + 1;

            terrain.indices[indexIndex++] = i0;
            terrain.indices[indexIndex++] = i1;
            terrain.indices[indexIndex++] = i2;

            terrain.indices[indexIndex++] = i1;
            terrain.indices[indexIndex++] = i3;
            terrain.indices[indexIndex++] = i2;

            assert(indexIndex <= terrain.index_count);
        }
    }
    assert(indexIndex == terrain.index_count);

    return terrain;
}

