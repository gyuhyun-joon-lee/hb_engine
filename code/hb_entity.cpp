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
    result->vertices = (VertexPN *)malloc(sizeof(VertexPN) * result->vertex_count);
    for(u32 i = 0;
            i < result->vertex_count;
            ++i)
    {
        result->vertices[i].p.x = cube_vertices[6*i+0];
        result->vertices[i].p.y = cube_vertices[6*i+1];
        result->vertices[i].p.z = cube_vertices[6*i+2];

        result->vertices[i].n.x = cube_vertices[6*i+3];
        result->vertices[i].n.y = cube_vertices[6*i+4];
        result->vertices[i].n.z = cube_vertices[6*i+5];
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

internal Entity *
add_floor_entity(GameState *game_state, MemoryArena *arena, v3 center, v2 dim, v3 color, u32 x_quad_count, u32 y_quad_count,
                 f32 max_height)
{
    Entity *result = add_entity(game_state, EntityType_Floor, Entity_Flag_Collides);

    // This is render p and dim, not the acutal dim
    result->p = center; 
    result->dim = V3(1, 1, 1);

    result->color = color;
    result->x_quad_count = x_quad_count;
    result->y_quad_count = y_quad_count;

    generate_floor_mesh(result, arena, dim, x_quad_count, y_quad_count, max_height);

    return result;
}

internal Entity *
add_sphere_entity(GameState *game_state, MemoryArena *arena, v3 center, f32 radius, v3 color)
{
    Entity *result = add_entity(game_state, EntityType_Sphere, Entity_Flag_Collides);

    result->p = center;
    result->dim = radius*V3(1, 1, 1);
    result->color = color;

    generate_sphere_mesh(result, &game_state->transient_arena, 1.0f, 256, 128);

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
                        VertexPN *vertices, u32 vertex_count, u32 hash, f32 time, f32 wiggliness)
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
            vertices[2*i].n = normal;
            vertices[2*i+1].n = normal;
        }
        else
        {
            // add tip
            vertices[vertex_count - 1].p = modified_p2 + 0.5f*blade_width*orthogonal_normal;
            vertices[vertex_count - 1].n = normalize(cross(quadratic_bezier_first_derivative(p0, modified_p1, modified_p2, 1.0f), orthogonal_normal));
        }
    }
}
