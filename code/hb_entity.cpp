/*
 * Written by Gyuhyun Lee
 */

#include "hb_entity.h"

// TODO(gh) Fixed particle radius, make this dynamic(might sacrifice stability)
#define particle_radius 1.0f

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

    f32 particle_diameter = 2.0f*particle_radius;
    u32 particle_x_count = ceil_f32_to_u32(dim.x / particle_diameter);
    u32 particle_y_count = ceil_f32_to_u32(dim.y / particle_diameter);
    u32 particle_z_count = ceil_f32_to_u32(dim.z / particle_diameter);

    u32 total_particle_count = particle_x_count *
                               particle_y_count *
                               particle_z_count;

    f32 inv_particle_mass = total_particle_count*inv_mass;

    start_particle_allocation_from_pool(&game_state->particle_pool, &result->particle_group);

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
                allocate_particle_from_pool(&game_state->particle_pool, 
                                            left_bottom_particle_center + particle_diameter*V3(x, y, z),
                                            particle_radius,
                                            inv_particle_mass);
            }

            y_index += particle_x_count;
        }

        z_index += particle_x_count*particle_y_count;
    }

    end_particle_allocation_from_pool(&game_state->particle_pool, &result->particle_group);

    return result;
}

internal void
add_distance_constraint(PBDParticleGroup *group, u32 index0, u32 index1)
{
    DistanceConstraint *c = group->distance_constraints + group->distance_constraint_count++;

    if(index0 > index1)
    {
        c->index0 = index1;
        c->index1 = index0;
    }
    else
    {
        c->index0 = index0;
        c->index1 = index1;
    }

    c->rest_length = length(group->particles[index0].p - group->particles[index1].p);
}

// NOTE(gh) p0 p1 p2 are the bottom plane vertices in counter clockwise order,
// p3 is the top vertex
internal f32
get_tetrahedron_volume(v3 p0, v3 p1, v3 p2, v3 p3)
{
    f32 result = (1/6.0f) * dot(cross(p1 - p0, p2 - p0), p3 - p0);

    return result;
}

internal void
add_volume_constraint(PBDParticleGroup *group, 
                     u32 index0, u32 index1, u32 index2, u32 index3)
{
    PBDParticle *particle0 = group->particles + index0;
    PBDParticle *particle1 = group->particles + index1;
    PBDParticle *particle2 = group->particles + index2;
    PBDParticle *particle3 = group->particles + index3;

    VolumeConstraint *c = group->volume_constraints + group->volume_constraint_count++;
    c->index0 = index0;
    c->index1 = index1;
    c->index2 = index2;
    c->index3 = index3;
    c->rest_volume = get_tetrahedron_volume(particle0->p, particle1->p, particle2->p, particle3->p);
}

// bottom 3 point should be in counter clockwise order
internal Entity *
add_pbd_soft_body_tetrahedron_entity(GameState *game_state, 
                                MemoryArena *arena,
                                v3 top,
                                v3 bottom_p0, v3 bottom_p1, v3 bottom_p2, 
                                v3 color, f32 inv_mass, u32 flags)
{
    Entity *result = add_entity(game_state, EntityType_SoftBody, flags);
    result->color = color;

    f32 inv_particle_mass = 4 * inv_mass;

    PBDParticleGroup *group = &result->particle_group;

    start_particle_allocation_from_pool(&game_state->particle_pool, group);

    allocate_particle_from_pool(&game_state->particle_pool, 
                                bottom_p0,
                                particle_radius,
                                inv_particle_mass);

    allocate_particle_from_pool(&game_state->particle_pool, 
                                bottom_p1,
                                particle_radius,
                                inv_particle_mass);

    allocate_particle_from_pool(&game_state->particle_pool, 
                                bottom_p2,
                                particle_radius,
                                inv_particle_mass);

    allocate_particle_from_pool(&game_state->particle_pool,
                                top,
                                particle_radius,
                                inv_particle_mass);

    end_particle_allocation_from_pool(&game_state->particle_pool, group);

    group->distance_constraints = push_array(arena, DistanceConstraint, 6);
    group->distance_constraint_count = 0;
    add_distance_constraint(group, 0, 1);
    add_distance_constraint(group, 0, 2);
    add_distance_constraint(group, 1, 2);
    add_distance_constraint(group, 0, 3);
    add_distance_constraint(group, 1, 3);
    add_distance_constraint(group, 2, 3);

    group->volume_constraints = push_array(arena, VolumeConstraint, 1);
    add_volume_constraint(group, 0, 1, 2, 3);

    return result;
}

// NOTE(gh) top p0 and p1 are the vertices perpendicular to the 
// bottom triangle
internal Entity *
add_pbd_soft_body_bipyramid_entity(GameState *game_state, 
                                MemoryArena *arena,
                                v3 top_p0, 
                                v3 bottom_p0, v3 bottom_p1, v3 bottom_p2,
                                v3 top_p1,
                                v3 color, f32 inv_mass, u32 flags)
{
    Entity *result = add_entity(game_state, EntityType_SoftBody, flags);
    result->color = color;

    u32 vertex_count = 5;
    f32 inv_particle_mass = vertex_count * inv_mass;

    PBDParticleGroup *group = &result->particle_group;

    start_particle_allocation_from_pool(&game_state->particle_pool, group);

    allocate_particle_from_pool(&game_state->particle_pool, 
                                bottom_p0,
                                particle_radius,
                                inv_particle_mass);

    allocate_particle_from_pool(&game_state->particle_pool, 
                                bottom_p1,
                                particle_radius,
                                inv_particle_mass);

    allocate_particle_from_pool(&game_state->particle_pool, 
                                bottom_p2,
                                particle_radius,
                                inv_particle_mass);

    allocate_particle_from_pool(&game_state->particle_pool,
                                top_p0,
                                particle_radius,
                                inv_particle_mass);

    allocate_particle_from_pool(&game_state->particle_pool, 
                                top_p1,
                                particle_radius,
                                inv_particle_mass);

    end_particle_allocation_from_pool(&game_state->particle_pool, group);

    group->distance_constraints = push_array(arena, DistanceConstraint, 9);
    group->distance_constraint_count = 0;
    add_distance_constraint(group, 0, 1);
    add_distance_constraint(group, 1, 2);
    add_distance_constraint(group, 0, 2);

    add_distance_constraint(group, 0, 3);
    add_distance_constraint(group, 1, 3);
    add_distance_constraint(group, 2, 3);

    add_distance_constraint(group, 0, 4);
    add_distance_constraint(group, 1, 4);
    add_distance_constraint(group, 2, 4);

    group->volume_constraints = push_array(arena, VolumeConstraint, 2);
    add_volume_constraint(group, 0, 1, 2, 3);
    // TODO(gh) This weird order is due to how we are constructing the vertices
    // dynamically
    add_volume_constraint(group, 0, 4, 2, 1);

    return result;
}

















