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

// NOTE(gh) floor is non_movable entity with infinite mass
internal Entity *
add_floor_entity(GameState *game_state, MemoryArena *arena, v3 p, v3 dim, v3 color)
{
    Entity *result = add_entity(game_state, EntityType_Floor, Entity_Flag_Collides);
    result->p = p;
    result->dim = dim;
    result->color = color;

    result->vertex_count = array_count(cube_vertices) / 6;
    result->vertices = push_array(arena, CommonVertex, result->vertex_count);
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
    result->indices = push_array(arena, u32, result->index_count);

    for(u32 i = 0;
            i < result->index_count;
            ++i)
    {
        result->indices[i] = i;
    }

    return result;
}

/*
   If the grass looks like this,
   /\
   ||
   ||
   ||
   ||
   grass_divided_count == 5
*/
internal void
populate_grass_vertices(v3 p0, f32 width, v2 facing_direction, 
                        f32 tilt, f32 bend, f32 grass_divided_count, 
                        CommonVertex *vertices, u32 vertex_count)
{
    assert(compare_with_epsilon(length_square(facing_direction), 1.0f));

    v3 p2 = V3(facing_direction, tilt); // TODO(gh) Am I using the tilt value correctly? 
    v3 orthogonal_normal = V3(-facing_direction.y, facing_direction.x, 0.0f); // Direction of the width of the grass blade

    v3 blade_normal = normalize(cross(p2 - p0, orthogonal_normal)); // normal of the p0 and p2, will be used to get p1 
    
    // TODO(gh) bend value is a bit unintuitive, because this is not represented in world unit.
    // But if bend value is 0, it means the grass will be completely flat
    v3 p1 = (3.0f/4.0f) * (p2 - p0) + bend * blade_normal;

    for(u32 i = 0;
            i < grass_divided_count; 
            ++i)
    {
        f32 t = (f32)i/(f32)grass_divided_count;

        v3 point_on_bezier_curve = quadratic_bezier(p0, p1, p2, t);

        // The first vertex is the point on the bezier curve,
        // and the next vertex is along the line that starts from the first vertex
        // and goes toward the orthogonal normal.
        // TODO(gh) Taper the grass blade down
        vertices[2*i].p = point_on_bezier_curve;
        vertices[2*i+1].p = point_on_bezier_curve + width * orthogonal_normal;

        v3 normal = normalize(cross(quadratic_bezier_first_derivative(p0, p1, p2, t), orthogonal_normal));
        vertices[2*i].normal = normal;
        vertices[2*i+1].normal = normal;
    }

    // Manually add the tip
    vertices[vertex_count - 1].p = p2;
    vertices[vertex_count - 1].normal = normalize(cross(quadratic_bezier_first_derivative(p0, p1, p2, 1.0f), orthogonal_normal));
}

internal Entity *
add_grass_entity(GameState *game_state, PlatformRenderPushBuffer *render_push_buffer, MemoryArena *arena, 
                 v3 p, f32 width, v3 color, 
                 v2 tilt_direction, f32 tilt, f32 bend, u32 grass_divided_count = 7)
{
    Entity *result = add_entity(game_state, EntityType_Grass, 0);
    result->p = p;
    result->dim = V3(1, 1, 1); // Does not have effect on the grass
    result->width = width;
    result->color = color;

    result->tilt = tilt;
    result->bend = bend;
    result->tilt_direction = tilt_direction;

    result->grass_divided_count = grass_divided_count;
    result->vertex_count = 2*result->grass_divided_count + 1;
    result->vertices = push_array(arena, CommonVertex, result->vertex_count); // TODO(gh) replace this with proper buffering

    result->index_count = 3 * (2*(result->grass_divided_count - 1)+1);
    result->indices = push_array(arena, u32, result->index_count);

    populate_grass_vertices(V3(0, 0, 0), result->width, result->tilt_direction, result->tilt, result->bend, result->grass_divided_count, 
                                        result->vertices, result->vertex_count);

    // populate indices
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
