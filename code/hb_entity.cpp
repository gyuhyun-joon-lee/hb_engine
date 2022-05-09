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

// NOTE(joon) floor is non_movable entity with infinite mass
internal Entity *
add_floor_entity(GameState *game_state, v3 p, v3 dim, v3 color)
{
    Entity *result = add_entity(game_state, Entity_Type_Floor, Entity_Flag_Collides);
    result->aabb = init_aabb(p, 0.5f * dim, 0.0f);

    result->color = color;
    
    return result;
}

internal Entity *
add_cube_rigid_body_entity(GameState *game_state, v3 p, v3 dim, f32 mass, v3 color)
{
    Entity *result = add_entity(game_state, Entity_Type_Cube, Entity_Flag_Movable|Entity_Flag_Collides);
    result->color = color;

    m3x3 intertia_tensor = 
        M3x3(mass*(dim.y*dim.y + dim.z*dim.z)/12.0f, 0, 0,
            0, mass*(dim.x*dim.x + dim.z*dim.z)/12.0f, 0,
            0, 0, mass*(dim.x*dim.x + dim.y*dim.y)/12.0f);

    result->rb = init_rigid_body(p, dim, safe_ratio(1.0f, mass), intertia_tensor);

    return result;
}

#if 0
internal Entity *
add_sphere_rigid_body_entity(GameState *game_state, v3 p, f32 r, f32 mass, v3 color)
{
    Entity *result = add_entity(game_state, Entity_Type_Cube, Entity_Flag_Movable|Entity_Flag_Collides);
    result->color = color;

    f32 c = (2.0f/5.0f) * mass * square(r);

    m3x3 intertia_tensor = c * M3x3();

    result->rb = init_rigid_body(p, );
}
#endif
