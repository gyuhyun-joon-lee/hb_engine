/*
 * Written by Gyuhyun Lee
 */

#include "hb_entity.h"

internal b32
is_entity_flag_set(u32 flags, EntityFlag flag)
{
    b32 result = false;
    if(flags & flag)
    {
        result = true;
    }
    
    return result;
}

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
add_floor_entity(GameState *game_state, MemoryArena *arena, v3 center, v2 dim, v3 color, u32 x_quad_count, u32 y_quad_count,
                 f32 max_height)
{
    Entity *result = add_entity(game_state, EntityType_Floor, EntityFlag_Collides);

    // This is render p and dim, not the acutal dim
    result->generic_entity_info.position = center; 
    result->generic_entity_info.dim = V3(dim, 1);

    result->color = color;

    return result;
}

internal Entity *
add_pbd_rigid_body_cube_entity(GameState *game_state, v3 center, v3 dim, v3 color, f32 inv_mass, u32 flags)
{
    Entity *result = add_entity(game_state, EntityType_Cube, flags);

    result->color = color;
    result->should_cast_shadow = true;

    // TODO(gh) Granularity of the particles?
    f32 particle_radius = 1.0f;
    f32 particle_diameter = 2.0f*particle_radius;
    u32 particle_x_count = ceil_f32_to_u32(dim.x / particle_diameter);
    u32 particle_y_count = ceil_f32_to_u32(dim.y / particle_diameter);
    u32 particle_z_count = ceil_f32_to_u32(dim.z / particle_diameter);

    u32 total_particle_count = particle_x_count *
                               particle_y_count *
                               particle_z_count;

    f32 inv_particle_mass = total_particle_count*inv_mass;

    result->particle_group = initialize_pbd_particle_group(&game_state->particle_pool, total_particle_count);

    // TODO(gh) This is only for the cube, but we want a generic voxelizing routine
    // for every mesh

    // NOTE(gh) This complicated equation comes from the fact that the 'center' should be different 
    // based on whether the particle count was even or odd.
    v3 left_bottom_particle_center = 
        center - particle_diameter * V3((particle_x_count-1)/2.0f,
                                      (particle_y_count-1)/2.0f,
                                      (particle_z_count-1)/2.0f);
    u32 z_index = 0;
    for(u32 z = 0;
            z < particle_z_count;
            ++z)
    {
        u32 y_index = 0;
        for(u32 y = 0;
                y < particle_y_count;
                ++y)
        {
            for(u32 x = 0;
                    x < particle_x_count;
                    ++x)
            {
                PBDParticle *particle = game_state->particle_pool.particles + 
                                        result->particle_group.start_index + 
                                        x + y_index + z_index;

                particle->position = left_bottom_particle_center + particle_diameter*V3(x, y, z);
                particle->velocity = V3(0, 0, 0);
                particle->radius = particle_radius;
                particle->inv_mass = inv_particle_mass;
                // TODO(gh) Not using the phase value for now
                particle->phase = 0;
            }

            y_index += particle_x_count;
        }

        z_index += particle_x_count*particle_y_count;
    }

    return result;
}

