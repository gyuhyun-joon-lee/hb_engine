/*
 * Written by Gyuhyun Lee
 */
// This where we custum generate the meshes

// latitude = horizontal, longitude = vertical
internal void
generate_sphere_mesh(Entity *entity, MemoryArena *arena, f32 r, u32 latitude_div_count, u32 longitude_div_count)
{
    entity->vertex_count = (longitude_div_count+1)*latitude_div_count;
    entity->vertices = push_array(arena, VertexPN, entity->vertex_count);

    entity->index_count = 2*3*longitude_div_count*latitude_div_count;
    entity->indices = push_array(arena, u32, entity->index_count);

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
            VertexPN *vertex = entity->vertices + populated_vertex_count;
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

                entity->indices[populated_index_count++] = i0;
                entity->indices[populated_index_count++] = i1;
                entity->indices[populated_index_count++] = i2;

                entity->indices[populated_index_count++] = i1;
                entity->indices[populated_index_count++] = i3;
                entity->indices[populated_index_count++] = i2;
            }

            populated_vertex_count++;
            latitude += latitude_advance_rad;
        }

        longitude += longitude_advance_rad;
    }

    assert(populated_vertex_count == entity->vertex_count);
    assert(populated_index_count == entity->index_count);
}
 
// NOTE(gh) The center of this is always (0, 0, 0),
// and the render code will offset the entire mesh using model matrix
internal void
generate_floor_mesh(Entity *entity, MemoryArena *arena, v2 dim, u32 x_quad_count, u32 y_quad_count)
{
    f32 quad_width = dim.x / (f32)x_quad_count;
    f32 quad_height = dim.y / (f32)y_quad_count;

    v2 left_bottom_p = -0.5f*dim;

    u32 x_vertex_count = x_quad_count + 1;
    u32 y_vertex_count = y_quad_count + 1;
    u32 total_vertex_count = x_vertex_count * y_vertex_count;

    entity->vertex_count = total_vertex_count;
    entity->vertices = push_array(arena, VertexPN, entity->vertex_count);

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

            f32 pz = 10.0f * perlin_noise01(frequency*xf, frequency*yf, 0, frequency);

            entity->vertices[populated_vertex_count].p = V3(px, py, pz);
            entity->vertices[populated_vertex_count].n = V3(0, 0, 0);

            populated_vertex_count++;
        }
    }
    assert(populated_vertex_count == entity->vertex_count);

    // three indices per triangle * 2 triangles per quad * total quad count
    entity->index_count = 3 * 2 * (x_quad_count * y_quad_count);
    entity->indices = push_array(arena, u32, entity->index_count);

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

            entity->indices[populated_index_count++] = i0;
            entity->indices[populated_index_count++] = i1;
            entity->indices[populated_index_count++] = i2;

            entity->indices[populated_index_count++] = i1;
            entity->indices[populated_index_count++] = i3;
            entity->indices[populated_index_count++] = i2;

            // TODO(gh) properly calculate normals(normal histogram?)
            v3 p0 = entity->vertices[i0].p;
            v3 p1 = entity->vertices[i1].p;
            v3 p2 = entity->vertices[i2].p;
            v3 p3 = entity->vertices[i3].p;

            v3 first_normal = normalize(cross(p1-p0, p2-p0));
            v3 second_normal = normalize(cross(p2-p3, p1-p3));

            entity->vertices[i0].n += first_normal;
            entity->vertices[i1].n += first_normal + second_normal;
            entity->vertices[i2].n += first_normal + second_normal;

            entity->vertices[i3].n = second_normal;
        }
    }
    assert(entity->index_count == populated_index_count);

    for(u32 vertex_index = 0;
            vertex_index < entity->vertex_count;
            ++vertex_index)
    {
        entity->vertices[vertex_index].n = normalize(entity->vertices[vertex_index].n);
    }
}
