#include "meka_entity.h"

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
add_voxel_entity(GameState *game_state, v3 p, u32 color)
{
    Entity *entity = add_entity(game_state, p, v3_(1, 1, 1), entity_type_voxel);
    entity->color = color;

    return entity;
}

internal Entity*
add_bullet_entity(GameState *game_state, v3 p, u32 color)
{
    Entity *entity = add_entity(game_state, p, v3_(1, 1, 1), entity_type_bullet);
    entity->color = color;

    return entity;
}

struct test_collision_point_plane_result
{
    b32 collided;
    b32 t;
};

// NOTE(joon) p0 and p1 are the points that are in the slabs 
// TDOO(joon) order of p0 and p1 is important?
internal test_collision_point_plane_result
test_collision_point_with_aa_slab(v3 slab_p0, v3 slab_p1, 
                                v3 delta)
{
    test_collision_point_plane_result result = {};
#if 0

    v3 t0 = hadamard((slab_p0 - ray_origin), inv_ray_dir); // each component represents t against the slab that is aligned for each axis
    v3 t1 = hadamard((slab_p1 - ray_origin), inv_ray_dir); // each component represents t against the slab that is aligned for each axis

    f32 max_t0 = max_component(t0);
    f32 min_t1 = min_component(t1);
    result.collided = (max_t0 <= min_t1);
    result.collide_p = max_t0;
#endif

    return result;
}

internal void
move_entity(GameState *game_state, Entity *entity, f32 dt_per_frame)
{
    // NOTE(joon) center of the voxel
    v3 p_delta = dt_per_frame * v3_(5, 5, 5);
    v3 new_p = entity->p = p_delta;

    v3 new_entity_corners[8] = 
    {
        {new_p.x + 0.5f, new_p.y + 0.5f, new_p.z + 0.5f},
        {new_p.x + 0.5f, new_p.y - 0.5f, new_p.z + 0.5f},
        {new_p.x - 0.5f, new_p.y + 0.5f, new_p.z + 0.5f},
        {new_p.x - 0.5f, new_p.y - 0.5f, new_p.z + 0.5f},

        {new_p.x + 0.5f, new_p.y + 0.5f, new_p.z - 0.5f},
        {new_p.x + 0.5f, new_p.y - 0.5f, new_p.z - 0.5f},
        {new_p.x - 0.5f, new_p.y + 0.5f, new_p.z - 0.5f},
        {new_p.x - 0.5f, new_p.y - 0.5f, new_p.z - 0.5f},
    };

    f32 min_t = 1.0f;
    for(u32 test_entity_index = 0;
            test_entity_index < game_state->entity_count;
            ++test_entity_index)
    {
        Entity *test_entity = game_state->entities + test_entity_index;

        f32 t = 1.0f;
        if(test_entity != entity)
        {
            for(u32 corner_index = 0;
                    corner_index < array_count(new_entity_corners);
                    ++corner_index)
            {
                //test_collision_point_with_aa_slab(v3 p0, v3 p1);

                if(min_t > t)
                {
                    min_t = t;
                }
            }
        }

    }
}
