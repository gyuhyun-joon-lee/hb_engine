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
add_floor_entity(GameState *game_state, v3 p, v3 dim, v3 color)
{
    Entity *result = add_entity(game_state, EntityType_Floor, Entity_Flag_Collides);
    result->p = p;
    result->dim = dim;
    result->color = color;

    result->vertices = (CommonVertex *)malloc(sizeof(CommonVertex) * result->vertex_count);
    result->vertex_count = array_count(cube_vertices) / 6;
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

// NOTE(gh) Grass uses quadratic bezier curve for now(original implementation uses cubic bezier curve for whatever reason).
// p0 is always {0, 0, 0}, p2 is {0, 1, 1}, and p1(midpoint) should be provided.  
// All three of them will be offset by the provided position of the grass.
internal Entity *
add_grass_entity(GameState *game_state, PlatformRenderPushBuffer *render_push_buffer, v3 p, v3 color, v3 p1)
{
    Entity *result = add_entity(game_state, EntityType_Grass, 0);
    result->p = p;
    result->color = color;
    result->p1 = p1;

    u32 grass_divide = 7;
    u32 grass_vertex_count = 2*grass_divide + 1; 

    v3 p0 = V3(0, 0, 0);
    v3 p2 = V3(0, 1, 1);

    result->vertices = (CommonVertex *)malloc(sizeof(CommonVertex) * grass_vertex_count); // TODO(gh) replace this with proper buffering
    result->vertex_count = grass_vertex_count;

    // populate vertices
    for(u32 i = 0;
            i < grass_divide; 
            ++i)
    {
        f32 t = (f32)i/(f32)grass_divide;

        v3 r = quadratic_bezier(p0, result->p1, p2, t);

        result->vertices[2*i].p = r;
        result->vertices[2*i].p.x = -0.5f;
        result->vertices[2*i+1].p = r;
        result->vertices[2*i+1].p.x = 0.5f;
    }
    // Manually add the tip
    result->vertices[grass_vertex_count - 1].p = p2;

    // TODO(gh) Also populate the normal

    u32 grass_index_count = 3 * (2*(grass_divide - 1)+1);
    result->indices = (u32 *)malloc(sizeof(u32) * grass_index_count);
    result->index_count = grass_index_count;

    // populate indices
    u32 populated_index_count = 0;
    for(u32 i = 0;
            i < grass_divide - 1; // the last point should be manually added, as it should be tappered down
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
    result->indices[populated_index_count++] = grass_vertex_count - 1;
    result->indices[populated_index_count++] = grass_vertex_count - 3;
    result->indices[populated_index_count++] = grass_vertex_count - 2;

    assert(populated_index_count == grass_index_count);

    return result;
}
