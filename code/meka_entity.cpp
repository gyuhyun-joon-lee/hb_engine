#include "meka_entity.h"

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

#if 0
internal Entity *
add_normalized_voxel_entity(GameState *game_state, v3 p, v3 color, f32 mass)
{
    v3 dim = V3(1, 1, 1);

    Entity *entity = add_entity(game_state, p, dim, mass, Entity_Type_Voxel);
    entity->color = color;

    entity->aabb = add_aabb(V3(), 0.5f*dim);

    //entity->rigid_body = ;

    return entity;
}

internal Entity *
add_normalized_voxel_entity(GameState *game_state, v3 p, u32 color_u32, f32 mass)
{
    v3 color = {};
    color.r = (f32)((color_u32 >> 0) & 0xff) / 255.0f;
    color.g = (f32)((color_u32 >> 8) & 0xff) / 255.0f;
    color.b = (f32)((color_u32 >> 16) & 0xff) / 255.0f;

    Entity *entity = add_normalized_voxel_entity(game_state, p, color, mass);
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

        add_normalized_voxel_entity(game_state, V3(x, y, z), color, 1.0f);
    }

    int a = 1;
}
#endif

internal Entity *
add_cube_mass_agg_entity(GameState *game_state, MemoryArena *arena, v3 center, v3 dim, v3 color, f32 total_mass, f32 elastic_value)
{
    // TODO(joon) test if mass is infinite, and set the entity flag accordingly
    Entity *result = add_entity(game_state, Entity_Type_Mass_Agg, Entity_Flag_Movable|Entity_Flag_Collides);
    result->mass_agg = init_cube_mass_agg(arena, center, dim, total_mass, elastic_value); 
    result->color = color;

    return result;
}

internal Entity *
add_mass_agg_entity_from_mesh(GameState *game_state, MemoryArena *arena, v3 center, v3 color, v3 *vertices, u32 vertex_count, u32 *indices, u32 index_count,
                              v3 scale, f32 total_mass, f32 elastic_value)
{
    Entity *result = add_entity(game_state, Entity_Type_Mass_Agg, Entity_Flag_Movable|Entity_Flag_Collides);

    result->mass_agg = init_mass_agg_from_mesh(arena, center, vertices, vertex_count, indices, index_count, scale, total_mass, elastic_value); 
    result->color = color;

    return result;
}

internal Entity *
add_flat_triangle_mass_agg_entity(GameState *game_state, MemoryArena *arena, v3 color, f32 total_mass, f32 elastic_value)
{
    Entity *result = add_entity(game_state, Entity_Type_Mass_Agg, Entity_Flag_Movable|Entity_Flag_Collides);
    result->mass_agg = init_flat_triangle_mass_agg(arena, total_mass, elastic_value); 
    result->color = color;

    return result;
}

internal Entity *
add_aabb_entity(GameState *game_state, v3 p, v3 dim, v3 color, f32 mass)
{
    // TODO(joon) test if mass is infinite, and set the entity flag accordingly
    Entity *result = add_entity(game_state, Entity_Type_Floor, Entity_Flag_Movable|Entity_Flag_Collides);
    result->color = color;
    result->aabb = init_aabb(p, 0.5f*dim, safe_ratio(1.0f, mass));

    return result;
}

// NOTE(joon) floor is non_movable entity with infinite mass
internal Entity *
add_floor_entity(GameState *game_state, v3 p, v3 dim, v3 color)
{
    Entity *result = add_entity(game_state, Entity_Type_Floor, Entity_Flag_Collides);
    result->aabb = init_aabb(p, 0.5f * dim, 0.0f);
    
    return result;
}




