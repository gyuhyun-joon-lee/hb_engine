#include "meka_entity.h"

#if 0
internal
add_aabb_collision()
{
}
#endif

internal AABB
add_aabb(v3 center, v3 half_dim)
{
    AABB result = {};

    result.center = center;
    result.half_dim = half_dim;

    return result;
}

internal Entity *
add_entity(GameState *game_state, v3 p, v3 dim, EntityType type)
{
    Entity *entity = game_state->entities + game_state->entity_count++;

    assert(game_state->entity_count <= game_state->max_entity_count);

    entity->p = p;
    entity->dim = dim;
    entity->type = type;

    return entity;
}

internal Entity *
add_normalized_voxel_entity(GameState *game_state, v3 p, v3 color)
{
    v3 dim = V3(1, 1, 1);

    Entity *entity = add_entity(game_state, p, dim, Entity_Type_Voxel);
    entity->color = color;

    entity->aabb = add_aabb(V3(), 0.5f*dim);

    return entity;
}

internal Entity *
add_normalized_voxel_entity(GameState *game_state, v3 p, u32 color_u32)
{
    v3 color = {};
    color.r = (f32)((color_u32 >> 0) & 0xff) / 255.0f;
    color.g = (f32)((color_u32 >> 8) & 0xff) / 255.0f;
    color.b = (f32)((color_u32 >> 16) & 0xff) / 255.0f;

    Entity *entity = add_normalized_voxel_entity(game_state, p, color);
    return entity;
}

internal void
add_voxel_entities_from_vox_file(GameState *game_state, load_vox_result vox)
{
    for(u32 voxel_index = 0;
            voxel_index < vox.voxel_count;
            ++voxel_index)
    {
        // TODO(joon) we also need to take account of the chunk pos?
        u8 x = vox.xs[voxel_index];
        u8 y = vox.ys[voxel_index];
        u8 z = vox.zs[voxel_index];
        u32 color = vox.palette[vox.colorIDs[voxel_index]];

        add_normalized_voxel_entity(game_state, V3(x, y, z), color);
    }

    int a = 1;
}

internal Entity *
add_room_entity(GameState *game_state, v3 p, v3 dim)
{
    Entity *entity = add_entity(game_state, p, dim, Entity_Type_Room);
    entity->color = V3(0.3f, 0.3f, 0.3f);

    // TODO(joon) : Do we want to use aabb here?
    entity->aabb = add_aabb(V3(), 0.5f*dim);

    return entity;
}

