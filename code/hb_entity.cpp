/*
 * Written by Gyuhyun Lee
 */

#include "hb_entity.h"

// TODO(gh) Fixed particle radius, make this dynamic(might sacrifice stability)
#define particle_radius 0.25f

internal b32
is_entity_flag_set(Entity *entity, EntityFlag flag)
{
    b32 result = false;
    if(entity->flags & flag)
    {
        result = true;
    }
    
    return result;
}

internal Entity *
add_entity(GameState *game_state, EntityType type, u32 flags)
{
    Entity *entity = game_state->entities + game_state->entity_count++;

    assert(game_state->entity_count <= array_count(game_state->entities));

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
add_floor_entity(GameState *game_state, v3 center, v2 dim, v3 color, u32 x_quad_count, u32 y_quad_count,
                 f32 max_height)
{
    Entity *result = add_entity(game_state, EntityType_Floor, EntityFlag_Collides);

    // This is render p and dim, not the acutal dim
    result->generic_entity_info.position = center; 
    result->generic_entity_info.dim = V3(dim, 1);

    result->color = color;

    return result;
}

internal void
add_distance_constraint(PBDParticleGroup *group, u32 index0, u32 index1)
{
    // TODO(gh) First, search through the constraints to see if there is a duplicate.
    // This is a very slow operation that scales horribly, so might be better if we 
    // can use maybe hashing??

    b32 should_add_new_constraint = true;
    for(u32 c_index = 0;
            c_index < group->distance_constraint_count;
            ++c_index)
    {
        DistanceConstraint *c = group->distance_constraints + c_index;
        if((c->index0 == index0 && c->index1 == index1) || 
          (c->index0 == index1 && c->index1 == index0))
        {
            should_add_new_constraint = false;
        }
    }

    if(should_add_new_constraint)
    {
        DistanceConstraint *c = group->distance_constraints + group->distance_constraint_count++;

        c->index0 = index0;
        c->index1 = index1;

        c->rest_length = length(group->particles[index0].p - group->particles[index1].p);
    }
}

internal void
add_volume_constraint(PBDParticleGroup *group, 
                     u32 top, u32 bottom0, u32 bottom1, u32 bottom2)
{
    PBDParticle *particle0 = group->particles + top;
    PBDParticle *particle1 = group->particles + bottom0;
    PBDParticle *particle2 = group->particles + bottom1;
    PBDParticle *particle3 = group->particles + bottom2;

    VolumeConstraint *c = group->volume_constraints + group->volume_constraint_count++;
    c->index0 = top;
    c->index1 = bottom0;
    c->index2 = bottom1;
    c->index3 = bottom2;
    c->rest_volume = get_tetrahedron_volume(particle0->p, particle1->p, particle2->p, particle3->p);
}

internal v9d
get_quadratic_deformation_q(v3d initial_offset_from_com)
{
    v9d result = V9d(initial_offset_from_com.x,
            initial_offset_from_com.y,
            initial_offset_from_com.z,
            square(initial_offset_from_com.x),
            square(initial_offset_from_com.y),
            square(initial_offset_from_com.z),
            initial_offset_from_com.x*initial_offset_from_com.y,
            initial_offset_from_com.y*initial_offset_from_com.z,
            initial_offset_from_com.z*initial_offset_from_com.x);

    return result;
}

// NOTE(gh) This has to happen after the particle allocation is done!!!
internal void
populate_pbd_shape_matching_info(Entity *entity, 
                                PBDParticleGroup *group,
                                f32 linear_deformation_c)

{
    group->linear_deformation_c = linear_deformation_c;

    v3d com = get_com_of_particle_group(group);
    for(u32 particle_index = 0;
            particle_index < group->count;
            ++particle_index)
    {
        PBDParticle *particle = group->particles + particle_index;
        particle->initial_offset_from_com = particle->p - com;
    }

    if(is_entity_flag_set(entity, EntityFlag_Quadratic) || 
        is_entity_flag_set(entity, EntityFlag_Linear))
    {
        // NOTE(gh) 
        // Aqq = inverse(mi * qi * transpose(qi)) where q if xi0 - com0, which should result in 3x3 matrix
        // Aqq should be a symmetric matrix, meaning that it contains only scaling but no rotation.
        m3x3d Aqq = M3x3d();
        for(u32 particle_index = 0;
                particle_index < group->count;
                ++particle_index)
        {
            PBDParticle *particle = group->particles + particle_index;

            assert(!compare_with_epsilon_f64(particle->inv_mass, 0.0));
            f64 particle_mass = 1.0/particle->inv_mass;

            Aqq.rows[0] += particle_mass * particle->initial_offset_from_com.x * particle->initial_offset_from_com;
            Aqq.rows[1] += particle_mass * particle->initial_offset_from_com.y * particle->initial_offset_from_com;
            Aqq.rows[2] += particle_mass * particle->initial_offset_from_com.z * particle->initial_offset_from_com;
        }

        // group->linear_shape_matching_coefficient = linear_shape_matching_coefficient;
        assert(is_inversable(Aqq) && is_symmetric(Aqq));
        group->linear_inv_Aqq = inverse(Aqq);

        if(is_entity_flag_set(entity, EntityFlag_Quadratic))
        {
            m9x9d quadratic_Aqq = {};
            for(u32 particle_index = 0;
                    particle_index < group->count;
                    ++particle_index)
            {
                PBDParticle *particle = group->particles + particle_index;

                // TODO(gh) We can store this, but the problem is that 
                // this is so huge
                v9d q = get_quadratic_deformation_q(particle->initial_offset_from_com);

                // NOTE(gh) This is equivalent to q * transpose(q),
                // which results with 9x9 matrix.
                quadratic_Aqq.rows[0] += q.e[0] * q;
                quadratic_Aqq.rows[1] += q.e[1] * q;
                quadratic_Aqq.rows[2] += q.e[2] * q;
                quadratic_Aqq.rows[3] += q.e[3] * q;
                quadratic_Aqq.rows[4] += q.e[4] * q;
                quadratic_Aqq.rows[5] += q.e[5] * q;
                quadratic_Aqq.rows[6] += q.e[6] * q;
                quadratic_Aqq.rows[7] += q.e[7] * q;
                quadratic_Aqq.rows[8] += q.e[8] * q;
            }

            // TODO(gh) Can Aqq even be non-inversable??
            // assert(is_inversable(Aqq));
            group->quadratic_inv_Aqq = inverse(quadratic_Aqq);
        }
    }
}


// TODO(gh) Later, we would want this is voxelize any mesh
// we throw in
internal Entity *
add_pbd_cube_entity(GameState *game_state, 
                    v3d center, v3d dim, 
                    v3d v,
                    f32 linear_deformation_c, 
                    f32 inv_mass, v3 color, u32 flags)
{
    Entity *result = add_entity(game_state, EntityType_PBD, flags);
    result->color = color;

    u32 vertex_count = 8;
    f32 inv_particle_mass = vertex_count * inv_mass;

    v3d left_bottom_corner = center - 0.5*dim + particle_radius*V3d(1, 1, 1);
    u32 particle_count_x = round_f64_to_u32(dim.x / particle_radius);
    u32 particle_count_y = round_f64_to_u32(dim.y / particle_radius);
    u32 particle_count_z = round_f64_to_u32(dim.z / particle_radius);

    PBDParticleGroup *group = &result->particle_group;
    start_particle_allocation_from_pool(&game_state->particle_pool, group);
    {
        for(i32 z = 0;
                z < particle_count_z;
                ++z)
        {
            for(i32 y = 0;
                    y < particle_count_y;
                    ++y)
            {
                for(i32 x = 0;
                        x < particle_count_x;
                        ++x)
                {
                    v3d p = left_bottom_corner + 2.0*particle_radius*V3d(x, y, z);
                    allocate_particle_from_pool(&game_state->particle_pool,
                                                p, v,
                                                particle_radius,
                                                inv_particle_mass);
                }
            }
        }
    }
    end_particle_allocation_from_pool(&game_state->particle_pool, group);

    populate_pbd_shape_matching_info(result, 
                                     group,
                                     linear_deformation_c);

    return result;
}

internal Entity *
add_pbd_vox_entity(GameState *game_state, 
                    LoadedVOXResult *loaded_vox,
                    v3d left_bottom_corner, 
                    v3d v,
                    f32 linear_deformation_c, f32 inv_mass, v3 color, u32 flags)
{
    Entity *result = add_entity(game_state, EntityType_PBD, flags);
    result->color = color;

    f32 inv_particle_mass = loaded_vox->voxel_count * inv_mass;

    PBDParticleGroup *group = &result->particle_group;
    start_particle_allocation_from_pool(&game_state->particle_pool, group);
    {
        for(u32 voxel_index = 0;
                voxel_index < loaded_vox->voxel_count;
                ++voxel_index)
        {
            v3d p = left_bottom_corner + 2.0f*particle_radius*
                        V3d(loaded_vox->xs[voxel_index], 
                            loaded_vox->ys[voxel_index], 
                            loaded_vox->zs[voxel_index]);
            allocate_particle_from_pool(&game_state->particle_pool,
                                        p, v,
                                        particle_radius,
                                        inv_particle_mass);
        }
    }
    end_particle_allocation_from_pool(&game_state->particle_pool, group);
    
    populate_pbd_shape_matching_info(result, 
                                     group,
                                     linear_deformation_c);

    return result;
}

internal Entity *
add_pbd_single_particle_entity(GameState *game_state, 
                                v3d p, 
                                v3d v,
                                f32 inv_edge_stiffness, f32 inv_mass, v3 color, u32 flags)
{
    Entity *result = add_entity(game_state, EntityType_PBD, flags);
    result->color = color;

    f32 inv_particle_mass = inv_mass;

    PBDParticleGroup *group = &result->particle_group;
    start_particle_allocation_from_pool(&game_state->particle_pool, group);
    {
        allocate_particle_from_pool(&game_state->particle_pool,
                                    p, v,
                                    particle_radius,
                                    inv_particle_mass);
    }
    end_particle_allocation_from_pool(&game_state->particle_pool, group);

    // Get COM to initialize initial_offset_from_com
    v3d com = get_com_of_particle_group(group);
    for(u32 particle_index = 0;
            particle_index < group->count;
            ++particle_index)
    {
        PBDParticle *particle = group->particles + particle_index;
        particle->initial_offset_from_com = particle->p - com;
    }

    group->inv_distance_stiffness = inv_edge_stiffness;

    return result;
}












































