#include "hb_entity.h"

#if 0
internal
add_aabb_collision()
{
}
#endif

internal Entity *
add_entity(GameState *game_state, EntityType type, u32 flags)
{
    Entity *entity = game_state->entities + game_state->entity_count++;

    assert(game_state->entity_count <= game_state->max_entity_count);

    entity->type = type;
    entity->flags = flags;

    return entity;
}

// NOTE(joon) floor is non_movable entity with infinite mass
internal Entity *
add_floor_entity(GameState *game_state, v3 p, v3 dim, v3 color)
{
    Entity *result = add_entity(game_state, EntityType_Floor, Entity_Flag_Collides);
    result->aabb = init_aabb(p, 0.5f * dim, 0.0f);

    result->color = color;
    
    return result;
}

internal Entity *
add_grass_entity(GameState *game_state, v3 p, v3 color)
{
    Entity *result = add_entity(game_state, EntityType_Grass, 0);
    result->p = p;
    result->color = color;

    return result;
}
