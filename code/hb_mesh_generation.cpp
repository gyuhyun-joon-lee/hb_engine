/*
 * Written by Gyuhyun Lee
 */
// This where we custum generate the meshes

internal void
generate_sphere_mesh(MemoryArena *arena, f32 r, u32 latitude_div_count, u32 longitude_div_count)
{
    u32 vertex_count = (longitude_div_count+1)*latitude_div_count;
    VertexPN *vertices = push_array(arena, VertexPN, vertex_count);

    u32 index_count = 2*3*longitude_div_count*latitude_div_count;
    u32 *indices = push_array(arena, u32, index_count);

    f32 latitude_advance_rad = (2.0f*pi_32) / latitude_div_count;
    f32 longitude_advance_rad = pi_32 / longitude_div_count;

    f32 longitude = -pi_32/2.0f;
    u32 populated_vertex_count = 0;
    u32 populated_index_count = 0;
    for(u32 y = 0;
            y < longitude_div_count+1; // +1 because unlike latitude, longitude does not loop
            ++y)
    {
        f32 latitude = 0.0f;
        f32 z = r*sinf(longitude);
        for(u32 x = 0;
                x < latitude_div_count;
                ++x)
        {
            vertices[populated_vertex_count++].p = V3(r*cosf(latitude)*cosf(longitude), 
                                                    r*sinf(latitude)*cosf(longitude), 
                                                    z);

            if(y != longitude_div_count)
            {
                /*
                   NOTE(gh) Given certain cycle, we will construct the mesh like this
                   v2-----v3
                   |       |
                   |       |
                   |       |
                   v0-----v1 -> indices : 012, 132
                */
                u32 i0 = populated_vertex_count;
                u32 i1 = latitude_div_count*y + (i0+1)%latitude_div_count; // loop
                u32 i2 = i0+latitude_div_count;
                u32 i3 = (latitude_div_count+1)*y + (i2+1)%latitude_div_count; // loop

                indices[populated_index_count++] = i0;
                indices[populated_index_count++] = i1;
                indices[populated_index_count++] = i2;

                indices[populated_index_count++] = i1;
                indices[populated_index_count++] = i3;
                indices[populated_index_count++] = i2;
            }

            latitude += latitude_advance_rad;
        }

        longitude += longitude_advance_rad;
    }

    assert(populated_vertex_count == vertex_count);
    assert(populated_index_count == index_count);
}
 
internal void
generate_floor_mesh()
{
}
