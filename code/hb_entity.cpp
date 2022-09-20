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
   If the grass looked like this,
   /\
   ||
   ||
   ||
   ||
   grass_divided_count == 5
*/
internal void
populate_grass_vertices(v3 p0, v3 p1, v3 p2, u32 grass_divided_count, CommonVertex *vertices, u32 vertex_count)
{
    for(u32 i = 0;
            i < grass_divided_count; 
            ++i)
    {
        f32 t = (f32)i/(f32)grass_divided_count;

        v3 r = quadratic_bezier(p0, p1, p2, t);

        vertices[2*i].p = r;
        vertices[2*i].p.x = -0.5f;
        vertices[2*i+1].p = r;
        vertices[2*i+1].p.x = 0.5f;
    }
    // Manually add the tip
    vertices[vertex_count - 1].p = p2;

    // TODO(gh) Also populate the normal

}

// NOTE(gh) Grass uses quadratic bezier curve for now(original implementation uses cubic bezier curve for whatever reason).
// p0 is always {0, 0, 0}, p2 is {0, 1, 1}, and p1(midpoint) should be provided.  
// All three of them will be offset by the provided position of the grass.
internal Entity *
add_grass_entity(GameState *game_state, PlatformRenderPushBuffer *render_push_buffer, MemoryArena *arena, v3 p, v3 dim, v3 color, v3 p1, u32 grass_divided_count = 7)
{
    Entity *result = add_entity(game_state, EntityType_Grass, 0);
    result->p = p;
    result->dim = dim;
    result->color = color;
    result->p1 = p1;
    result->p2_bob_dt = 0.0f; 

    result->grass_divided_count = grass_divided_count;
    result->vertex_count = 2*result->grass_divided_count + 1;
    result->vertices = push_array(arena, CommonVertex, result->vertex_count); // TODO(gh) replace this with proper buffering

    result->index_count = 3 * (2*(result->grass_divided_count - 1)+1);
    result->indices = push_array(arena, u32, result->index_count);

    v3 p2 = V3(0, 1, 1); // TODO(gh) also make this configurable?

    populate_grass_vertices(V3(0, 0, 0), result->p1, p2, result->grass_divided_count, 
                                        result->vertices, result->vertex_count);

    // populate indices
    u32 populated_index_count = 0;
    for(u32 i = 0;
            i < result->grass_divided_count - 1; // the last point should be manually added, as it should be tappered down
            ++i)
    {
        result->indices[populated_index_count++] = 2*i + 0;
        result->indices[populated_index_count++] = 2*i + 1;
        result->indices[populated_index_count++] = 2*i + 2;

        result->indices[populated_index_count++] = 2*i + 1;
        result->indices[populated_index_count++] = 2*i + 3;
        result->indices[populated_index_count++] = 2*i + 2;
    }

    // Add the tip triangle
    result->indices[populated_index_count++] = result->vertex_count - 1;
    result->indices[populated_index_count++] = result->vertex_count - 3;
    result->indices[populated_index_count++] = result->vertex_count - 2;

    assert(populated_index_count == result->index_count);

    return result;
}
