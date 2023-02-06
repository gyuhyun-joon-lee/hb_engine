/*
 * Written by Gyuhyun Lee
 */
// This where we custum generate the meshes

struct GeneratedMesh
{
    VertexPN *vertices;
    u32 vertex_count;

    u32 *indices;
    u32 index_count;
};

// latitude = horizontal, longitude = vertical
internal GeneratedMesh
generate_sphere_mesh(TempMemory *temp_memory, f32 r, u32 latitude_div_count, u32 longitude_div_count)
{
    GeneratedMesh result = {};

    result.vertex_count = (longitude_div_count+1)*latitude_div_count;
    result.index_count = 2*3*longitude_div_count*latitude_div_count;

    u32 total_vertex_buffer_size = sizeof(VertexPN)*result.vertex_count;
    u32 total_index_buffer_size = sizeof(u32)*result.index_count;

    result.vertices = (VertexPN *)push_size(temp_memory, total_vertex_buffer_size);
    result.indices = (u32 *)push_size(temp_memory, total_index_buffer_size);

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
            VertexPN *vertex = result.vertices + populated_vertex_count;
            vertex->p = V3(r*cosf(latitude)*cosf(longitude), 
                            r*sinf(latitude)*cosf(longitude), 
                            z);
            vertex->n = normalize(vertex->p);

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
                u32 i3 = latitude_div_count*(y+1) + (i2+1)%latitude_div_count; // loop

                result.indices[populated_index_count++] = i0;
                result.indices[populated_index_count++] = i1;
                result.indices[populated_index_count++] = i2;

                result.indices[populated_index_count++] = i1;
                result.indices[populated_index_count++] = i3;
                result.indices[populated_index_count++] = i2;
            }

            populated_vertex_count++;
            latitude += latitude_advance_rad;
        }

        longitude += longitude_advance_rad;
    }

    assert(populated_vertex_count == result.vertex_count);
    assert(populated_index_count == result.index_count);

    return result;
}
 
// NOTE(gh) The center of this is always (0, 0, 0),
// and the render code will offset the entire mesh using model matrix - and the floor ranges from
// -0.5f*grid_dim to 0.5f*grid_dim
internal GeneratedMesh
generate_floor_mesh(TempMemory *temp_memory, u32 x_quad_count, u32 y_quad_count,
                    f32 max_height)
{
    GeneratedMesh result = {};

    v2 floor_dim = V2(1, 1);

    f32 quad_width = floor_dim.x / (f32)x_quad_count;
    f32 quad_height = floor_dim.y / (f32)y_quad_count;

    v2 left_bottom_p = -0.5f*floor_dim;

    u32 x_vertex_count = x_quad_count + 1;
    u32 y_vertex_count = y_quad_count + 1;
    u32 total_vertex_count = x_vertex_count * y_vertex_count;

    result.vertex_count = total_vertex_count;
    result.vertices = push_array(temp_memory, VertexPN, result.vertex_count);

    u32 populated_vertex_count = 0;
    for(u32 y = 0;
            y < y_vertex_count;
            ++y)
    {
        f32 py = left_bottom_p.y + y * quad_height;
        f32 yf = (f32)y/(f32)(y_vertex_count-1); // -1 so that xf range from 0 to 1

        for(u32 x = 0;
                x < x_vertex_count;
                ++x)
        {
            f32 px = left_bottom_p.x + x * quad_width;
            f32 xf = (f32)x/((f32)x_vertex_count - 1); // -1 so that xf range from 0 to 1
            u32 frequency = 4;

            f32 pz = max_height * perlin_noise01(frequency*xf, frequency*yf, 0, frequency);

            result.vertices[populated_vertex_count].p = V3(px, py, pz);
            result.vertices[populated_vertex_count].n = V3(0, 0, 0);

            populated_vertex_count++;
        }
    }
    assert(populated_vertex_count == result.vertex_count);

    // three indices per triangle * 2 triangles per quad * total quad count
    result.index_count = 3 * 2 * (x_quad_count * y_quad_count);
    result.indices = push_array(temp_memory, u32, result.index_count);

    u32 populated_index_count = 0;
    for(u32 y = 0;
            y < y_quad_count;
            ++y)
    {
        for(u32 x = 0;
                x < x_quad_count;
                ++x)
        {
            /*
               NOTE(gh) Given certain cycle, we will construct the mesh like this
               v2-----v3
               |       |
               |       |
               |       |
               v0-----v1 -> indices : 012, 132
            */
            u32 i0 = y*(x_quad_count + 1) + x;
            u32 i1 = i0 + 1;
            u32 i2 = i0 + x_quad_count + 1;
            u32 i3 = i2 + 1;

            result.indices[populated_index_count++] = i0;
            result.indices[populated_index_count++] = i1;
            result.indices[populated_index_count++] = i2;

            result.indices[populated_index_count++] = i1;
            result.indices[populated_index_count++] = i3;
            result.indices[populated_index_count++] = i2;

            // TODO(gh) properly calculate normals(normal histogram?)
            v3 p0 = result.vertices[i0].p;
            v3 p1 = result.vertices[i1].p;
            v3 p2 = result.vertices[i2].p;
            v3 p3 = result.vertices[i3].p;

            v3 first_normal = normalize(cross(p1-p0, p2-p0));
            v3 second_normal = normalize(cross(p2-p3, p1-p3));

            result.vertices[i0].n += first_normal;
            result.vertices[i1].n += first_normal + second_normal;
            result.vertices[i2].n += first_normal + second_normal;

            result.vertices[i3].n = second_normal;
        }
    }
    assert(result.index_count == populated_index_count);

    for(u32 vertex_index = 0;
            vertex_index < result.vertex_count;
            ++vertex_index)
    {
        result.vertices[vertex_index].n = normalize(result.vertices[vertex_index].n);
    }

    return result;
}
