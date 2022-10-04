/*
 * Written by Gyuhyun Lee
 */

#include "hb_entity.h"

internal Entity *
add_entity(GameState *game_state, EntityType type, u32 flags)
{
    Entity *entity = game_state->entities + game_state->entity_count++;

    assert(game_state->entity_count <= game_state->max_entity_count);

    entity->type = type;
    entity->flags = flags;

    return entity;
}

f32 cube_vertices[] = 
{
    // -x
    -0.5f,-0.5f,-0.5f,  -1, 0, 0,
    -0.5f,-0.5f, 0.5f,  -1, 0, 0,
    -0.5f, 0.5f, 0.5f,  -1, 0, 0,

    // -z
    0.5f, 0.5f,-0.5f,  0, 0, -1,
    -0.5f,-0.5f,-0.5f,  0, 0, -1,
    -0.5f, 0.5f,-0.5f,  0, 0, -1,

    // -y
    0.5f,-0.5f, 0.5f,  0, -1, 0,
    -0.5f,-0.5f,-0.5f,  0, -1, 0,
    0.5f,-0.5f,-0.5f,  0, -1, 0,

    // -z
    0.5f, 0.5f,-0.5f,  0, 0, -1,
    0.5f,-0.5f,-0.5f,  0, 0, -1,
    -0.5f,-0.5f,-0.5f,  0, 0, -1,

    // -x
    -0.5f,-0.5f,-0.5f,  -1, 0, 0,
    -0.5f, 0.5f, 0.5f,  -1, 0, 0,
    -0.5f, 0.5f,-0.5f,  -1, 0, 0,

    // -y
    0.5f,-0.5f, 0.5f,  0, -1, 0,
    -0.5f,-0.5f, 0.5f,  0, -1, 0,
    -0.5f,-0.5f,-0.5f,  0, -1, 0,

    // +z
    -0.5f, 0.5f, 0.5f,  0, 0, 1,
    -0.5f,-0.5f, 0.5f,  0, 0, 1,
    0.5f,-0.5f, 0.5f,  0, 0, 1,

    // +x
    0.5f, 0.5f, 0.5f,  1, 0, 0,
    0.5f,-0.5f,-0.5f,  1, 0, 0,
    0.5f, 0.5f,-0.5f,  1, 0, 0,

    // +x
    0.5f,-0.5f,-0.5f,  1, 0, 0,
    0.5f, 0.5f, 0.5f,  1, 0, 0,
    0.5f,-0.5f, 0.5f,  1, 0, 0,

    // +y
    0.5f, 0.5f, 0.5f,  0, 1, 0,
    0.5f, 0.5f,-0.5f,  0, 1, 0,
    -0.5f, 0.5f,-0.5f,  0, 1, 0,

    // +y
    0.5f, 0.5f, 0.5f,  0, 1, 0,
    -0.5f, 0.5f,-0.5f,  0, 1, 0,
    -0.5f, 0.5f, 0.5f,  0, 1, 0,

    // +z
    0.5f, 0.5f, 0.5f,  0, 0, 1,
    -0.5f, 0.5f, 0.5f,  0, 0, 1,
    0.5f,-0.5f, 0.5f,   0, 0, 1,
};

internal Entity *
add_cube_entity(GameState *game_state, v3 p, v3 dim, v3 color)
{
    Entity *result = add_entity(game_state, EntityType_Cube, Entity_Flag_Collides);
    result->aabb = init_aabb(p, 0.5f * dim, 0.0f);

    result->p = p;
    result->dim = dim;
    result->color = color;

    result->vertex_count = array_count(cube_vertices) / 6;
    result->vertices = (CommonVertex *)malloc(sizeof(CommonVertex) * result->vertex_count);
    for(u32 i = 0;
            i < result->vertex_count;
            ++i)
    {
        result->vertices[i].p.x = cube_vertices[6*i+0];
        result->vertices[i].p.y = cube_vertices[6*i+1];
        result->vertices[i].p.z = cube_vertices[6*i+2];

        result->vertices[i].normal.x = cube_vertices[6*i+3];
        result->vertices[i].normal.y = cube_vertices[6*i+4];
        result->vertices[i].normal.z = cube_vertices[6*i+5];
    }

    result->index_count = result->vertex_count;
    result->indices = (u32 *)malloc(sizeof(u32) * result->index_count);

    for(u32 i = 0;
            i < result->index_count;
            ++i)
    {
        result->indices[i] = i;
    }

    return result;
}

// NOTE(gh) creates 512 x 512 grid
internal Entity *
add_floor_entity(GameState *game_state, MemoryArena *arena, v3 center, v2 dim, v3 color)
{
    Entity *result = add_entity(game_state, EntityType_Floor, Entity_Flag_Collides);
    result->p = center;
    result->dim = V3(dim, 0);
    result->color = color;

    u32 x_quad_count = 512;
    u32 y_quad_count = 512;

    // TODO(gh) Also make this configurable?
    f32 quad_width = dim.x / (f32)x_quad_count;
    f32 quad_height = dim.y / (f32)y_quad_count;

    u32 x_vertex_count = x_quad_count + 1;
    u32 y_vertex_count = y_quad_count + 1;
    u32 total_vertex_count = x_vertex_count * y_vertex_count;

    result->vertex_count = total_vertex_count;
    result->vertices = push_array(arena, CommonVertex, result->vertex_count);

    v3 left_bottom_p = center - V3(dim.x/2, dim.y/2, 0);

    u32 populated_vertex_count = 0;
    for(u32 y = 0;
            y < y_vertex_count;
            ++y)
    {
        f32 py = left_bottom_p.y + y * quad_height;
        f32 yf = (f32)y/(f32)y_vertex_count;

        for(u32 x = 0;
                x < x_vertex_count;
                ++x)
        {
            f32 px = left_bottom_p.x + x * quad_width;
            f32 xf = (f32)x/(f32)x_vertex_count;
            f32 factor = 8.0f;

            f32 pz = 10.0f * perlin_noise01(factor * xf, factor * yf, 0);

            result->vertices[populated_vertex_count].p = V3(px, py, pz);
            result->vertices[populated_vertex_count].normal = V3(0, 0, 0);

            populated_vertex_count++;
        }
    }
    assert(populated_vertex_count == result->vertex_count);

    // three indices per triangle * 2 triangles per quad * total quad count
    result->index_count = 3 * 2 * (x_quad_count * y_quad_count);
    result->indices = push_array(arena, u32, result->index_count);

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
               NOTE/gh: Given certain cycle, we will construct the mesh like this
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

            result->indices[populated_index_count++] = i0;
            result->indices[populated_index_count++] = i1;
            result->indices[populated_index_count++] = i2;

            result->indices[populated_index_count++] = i1;
            result->indices[populated_index_count++] = i3;
            result->indices[populated_index_count++] = i2;

            // TODO(gh) properly calculate normals(normal histogram?)
            v3 p0 = result->vertices[i0].p;
            v3 p1 = result->vertices[i1].p;
            v3 p2 = result->vertices[i2].p;
            v3 p3 = result->vertices[i3].p;

            v3 first_normal = normalize(cross(p1-p0, p2-p0));
            v3 second_normal = normalize(cross(p2-p3, p1-p3));

            result->vertices[i0].normal += first_normal;
            result->vertices[i1].normal += first_normal + second_normal;
            result->vertices[i2].normal += first_normal + second_normal;

            result->vertices[i3].normal = second_normal;
        }
    }
    assert(result->index_count == populated_index_count);

    for(u32 vertex_index = 0;
            vertex_index < result->vertex_count;
            ++vertex_index)
    {
        result->vertices[vertex_index].normal = normalize(result->vertices[vertex_index].normal);
    }

    return result;
}


/*
   If the grass blade looks like this,
   /\
   ||
   ||
   ||
   ||
   ||
   ||
   grass_divided_count == 7
*/
internal void
populate_grass_vertices(v3 p0, 
                        f32 blade_width, // how wide the width of the blade should be
                        f32 stride, // looking from the side, how wide the grass should go
                        f32 height, // how tall the grass should be
                        v2 facing_direction, 
                        f32 tilt, f32 bend, f32 grass_divided_count, 
                        CommonVertex *vertices, u32 vertex_count, u32 hash, f32 time, f32 wiggliness)
{
    assert(compare_with_epsilon(length_square(facing_direction), 1.0f));

    v3 p2 = p0 + stride * V3(facing_direction, 0.0f) + V3(0, 0, height);  
    v3 orthogonal_normal = normalize(V3(-facing_direction.y, facing_direction.x, 0.0f)); // Direction of the width of the grass blade

    v3 blade_normal = normalize(cross(p2 - p0, orthogonal_normal)); // normal of the p0 and p2, will be used to get p1 
    
    f32 hash_value = hash*pi_32;

    // TODO(gh) bend value is a bit unintuitive, because this is not represented in world unit.
    // But if bend value is 0, it means the grass will be completely flat
    v3 p1 = p0 + (2.6f/4.0f) * (p2 - p0) + bend * blade_normal;

    for(u32 i = 0;
            i <= grass_divided_count; 
            ++i)
    {
        f32 t = (f32)i/(f32)grass_divided_count;

        f32 wind_factor = t*wiggliness + hash_value + time;

#if 0
        // p1 += V3(0, 0, 0.05f * sinf(wind_factor));
        p1 += 0.1f * sinf(wind_factor) * blade_normal;
        p2 += V3(0, 0, 0.2f * sinf(wind_factor));

        v3 modified_p1 = p1;
        v3 modified_p2 = p2;

        v3 point_on_bezier_curve = quadratic_bezier(p0, p1, p2, t);
#else 
        v3 modified_p1 = p1 + V3(0, 0, 0.1f * sinf(wind_factor));
        v3 modified_p2 = p2 + V3(0, 0, 0.3f * sinf(wind_factor));

        v3 point_on_bezier_curve = quadratic_bezier(p0, modified_p1, modified_p2, t);
#endif

        if(i < grass_divided_count)
        {
            // The first vertex is the point on the bezier curve,
            // and the next vertex is along the line that starts from the first vertex
            // and goes toward the orthogonal normal.
            // TODO(gh) Taper the grass blade down
            vertices[2*i].p = point_on_bezier_curve;
            vertices[2*i+1].p = point_on_bezier_curve + blade_width * orthogonal_normal;

            v3 normal = normalize(cross(quadratic_bezier_first_derivative(p0, modified_p1, modified_p2, t), orthogonal_normal));
            vertices[2*i].normal = normal;
            vertices[2*i+1].normal = normal;
        }
        else
        {
            // add tip
            vertices[vertex_count - 1].p = modified_p2 + 0.5f*blade_width*orthogonal_normal;
            vertices[vertex_count - 1].normal = normalize(cross(quadratic_bezier_first_derivative(p0, modified_p1, modified_p2, 1.0f), orthogonal_normal));
        }
    }
}

internal Entity *
add_grass_entity(GameState *game_state, RandomSeries *series,
                 PlatformRenderPushBuffer *render_push_buffer, MemoryArena *arena, 
                 v3 p, f32 blade_width, f32 stride, f32 height, v3 color, 
                 v2 tilt_direction, f32 tilt, f32 bend, u32 grass_divided_count = 7)
{
    Entity *result = add_entity(game_state, EntityType_Grass, 0);
    result->p = p;
    result->dim = V3(1, 1, 1); // Does not have effect on the grass

    result->blade_width = blade_width;
    result->stride = stride;
    result->height = height;
    result->hash = random_between_u32(series, 0, 100);

    result->color = color;

    result->tilt = tilt;
    result->bend = bend;
    result->tilt_direction = tilt_direction;

    result->grass_divided_count = grass_divided_count;
    result->vertex_count = 2*result->grass_divided_count + 1;
    result->vertices = push_array(arena, CommonVertex, result->vertex_count); // TODO(gh) replace this with proper buffering

    result->index_count = 3 * (2*(result->grass_divided_count - 1)+1);
    result->indices = push_array(arena, u32, result->index_count);

    // This will be done in entity update loop anyway
#if 0
    populate_grass_vertices(p, result->width, result->tilt_direction, result->tilt, result->bend, result->grass_divided_count, 
                                        result->vertices, result->vertex_count, );
#endif

    // NOTE(gh) Normals are facing the generally opposite direction of tilt direction
    /*
        ||
       3  2
        ||
       1  0
    */
    u32 populated_index_count = 0;
    for(u32 i = 0;
            i < result->grass_divided_count - 1; // tip of the grass will be added later
            ++i)
    {
        result->indices[populated_index_count++] = 2*i + 0;
        result->indices[populated_index_count++] = 2*i + 3;
        result->indices[populated_index_count++] = 2*i + 1;

        result->indices[populated_index_count++] = 2*i + 0;
        result->indices[populated_index_count++] = 2*i + 2;
        result->indices[populated_index_count++] = 2*i + 3;
    }

    // Add the tip triangle
    result->indices[populated_index_count++] = result->vertex_count - 1;
    result->indices[populated_index_count++] = result->vertex_count - 3;
    result->indices[populated_index_count++] = result->vertex_count - 2;

    assert(populated_index_count == result->index_count);

    return result;
}

#if 0
internal void
plant_grasses_using_white_noise(GameState *game_state, RandomSeries *series, PlatformRenderPushBuffer *platform_render_push_buffer, MemoryArena *arena, 
                                f32 width, f32 height, f32 z, u32 desired_grass_count)
{
    f32 half_width = 0.5f*width;
    f32 half_height = 0.5f*height;
    RandomSeries random_series = start_random_series(121623);

    for(u32 grass_index = 0;
            grass_index < desired_grass_count;
            ++grass_index)
    {
        f32 tilt_direction_rad = random_between(series, 0.0f, tau_32);
        v2 tilt_direction = V2(cosf(tilt_direction_rad), sinf(tilt_direction_rad));

        add_grass_entity(game_state, series, platform_render_push_buffer, arena, 
                        V3(random_between(&random_series, -half_width, half_width), random_between(&random_series, -half_height, half_height), z), 
                        0.2f, 0.5f, 1.0f, 
                        V3(0, 1, 0.2f),
                        tilt_direction, random_between(&random_series, 0.5f, 1.0f), random_between(&random_series, 0.2f, 0.5f));
    }
}
#endif

internal void
plant_grasses_using_brute_force_blue_noise(GameState *game_state, RandomSeries *series, 
                                            PlatformRenderPushBuffer *platform_render_push_buffer, MemoryArena *arena, 
                                           f32 width, f32 height, f32 z, u32 desired_grass_count, f32 desired_distance_between)
{
    f32 half_width = 0.5f*width;
    f32 half_height = 0.5f*height;

    u32 first_grass_entity_index = game_state->entity_count;

    for(u32 grass_index = 0;
            grass_index < desired_grass_count;
            ++grass_index)
    {
        u32 max_try_count = 1000;
        b32 planted = false;
        while(!planted && 
              (max_try_count > 0))
        {
            f32 px = random_between(series, -half_width, half_width);
            f32 py = random_between(series, -half_height, half_height);
            f32 pz = z;
            // TODO(gh) f32 pz = raycast_to_plane_z();
            v3 p = V3(px, py, pz); 

            b32 distance_is_ok = true;
            // check the distance to all the grass entities that had been planted
            for(u32 i = 0;
                    i < grass_index;
                    ++i)
            {
                Entity *grass = game_state->entities + first_grass_entity_index + i;
                if(length_square(grass->p - p) < desired_distance_between)
                {
                    distance_is_ok = false;
                    break;
                }
            }

            if(distance_is_ok)
            {
                f32 tilt_direction_rad = random_between(series, 0.0f, tau_32);
                v2 tilt_direction = V2(cosf(tilt_direction_rad), sinf(tilt_direction_rad));
            
                f32 blade_width = 0.2f;
                f32 stride = 2.0f;
                f32 height = 1.9f;
                add_grass_entity(game_state, series, platform_render_push_buffer, arena, 
                                p, 
                                blade_width, stride, height, 
                                V3(0, 0.8f, 0.2f),
                                tilt_direction, random_between(series, 0.1f*height, 0.9f*height), random_between(series, 0.5f, 1.0f));

                planted = true;
            }

            max_try_count--;
        }
    }

    u32 one_past_last_grass_entity_index = game_state->entity_count;
    u32 planted_grass_count = one_past_last_grass_entity_index - first_grass_entity_index;
    // NOTE(gh) Not a required assert, just out of curiosity
    // assert(planted_grass_count == desired_grass_count);

    int a = 1;
}

