#include "meka_simulation.h"
/*
   NOTE(joon):
   1. v3(0, 0, 0) inside the simulation space is literally the origin of the axises. 
*/


internal AABB
set_aabb(v3 center, v3 half_dim)
{
    AABB result = {};

    result.center = center;
    result.half_dim = half_dim;

    return result;
}

#if 0
internal void
init_rigidbody(RigidBody *rigid_body, f32 mass, v3 p, )
{
    *rigid_body = {};

    // constant
    rigid_body->mass = mass;
    rigid_body->p = p;

    rigid->rotation.rows[0] = V3(1, 0, 0); 
    rigid->rotation.rows[1] = V3(0, 1, 0); 
    rigid->rotation.rows[2] = V3(0, 0, 1); 

    // TODO(joon) need some kind of mass density function
    // to get this?
    rigid_body->I_body = ;
    rigid_body->inv_I_body;

    // state
    //rigid_body->rotation;
    //rigid_body->P; // linear momentum, P = m * v, P' = F = m * a
    //rigid_body->L; // angular momentum, L = I * w

    // derived
    //rigid_body->inv_I; // I = R * I_body * RT, inv_I = R * inv_I_body * RT
    //rigid_body->v; // linear velocity
    //rigid_body->w; // angular velocity

    // computed
    //rigid_body->F; // force
    //rigid_body->T; // torque

}
#endif

// TODO(joon) local space aabb testing?
internal b32
test_aabb_aabb(AABB *a, AABB *b)
{
    b32 result = false;
    v3 half_dim_sum = a->half_dim + b->half_dim; 
    v3 rel_a_center = a->center - b->center;

    // TODO(joon) early return is faster?
    if((rel_a_center.x >= -half_dim_sum.x && rel_a_center.x < half_dim_sum.x) && 
       (rel_a_center.y >= -half_dim_sum.y && rel_a_center.y < half_dim_sum.y) && 
       (rel_a_center.z >= -half_dim_sum.z && rel_a_center.z < half_dim_sum.z))
    {
        result = true;
    }

    return result;
}

internal v3
compute_force(f32 mass, v3 a)
{
    v3 result = {};
    result = mass * a;

    return result;
}

internal void
move_entity(GameState *game_state, Entity *entity, f32 dt_per_frame)
{
    v3 force = compute_force(entity->mass, V3(0, 0, -9.8f));
    entity->v = entity->v + dt_per_frame * (force/entity->mass);

    v3 p_delta = dt_per_frame * entity->v;
    v3 new_p = entity->p + p_delta;
    AABB entity_aabb = entity->aabb;
    entity_aabb.center += entity->p;


    b32 collided = false;
    for(u32 test_entity_index = 0;
            test_entity_index < game_state->entity_count;
            ++test_entity_index)
    {
        Entity *test_entity = game_state->entities + test_entity_index;

        if(test_entity != entity)
        {
            // TODO(joon) collision rule hash!
            if(entity->type == Entity_Type_Voxel && 
                test_entity->type == Entity_Type_Voxel)
            {
                AABB test_entity_aabb = test_entity->aabb;
                test_entity_aabb.center += test_entity->p;
                
                collided = test_aabb_aabb(&entity_aabb, &test_entity_aabb);
            }
            else if(entity->type == Entity_Type_Voxel && 
                    test_entity->type == Entity_Type_Room)
            {
                AABB test_entity_aabb = test_entity->aabb;
                test_entity_aabb.center += test_entity->p;

                collided = !test_aabb_aabb(&entity_aabb, &test_entity_aabb);
            }
        }

        if(collided)
        {
            break;
        }
    }

    if(!collided)
    {
        entity->p = new_p;
    }
}

internal void
move_mass_aggregation(GameState *game_state, MassAggregation *agg, f32 dt_per_frame)
{
    // TODO(joon) drag should vary from entity to entity
    f32 drag = 0.9f;
    // NOTE(joon) applying gravity is really easy. We already know what the acceleration is,
    // so no need to change it to force.
    for(u32 particle_index = 0;
            particle_index < agg->particle_count;
            ++particle_index)
    {
        MassParticle *particle = agg->particles + particle_index;
        particle->v.z -= 9.8f * dt_per_frame;
        particle->v *= drag; // TODO(joon) should we do this before testing the collisions, or after collisions
    }

    for(u32 particle_index = 0;
            particle_index < agg->particle_count;
            ++particle_index)
    {
        MassParticle *particle = agg->particles + particle_index;
        particle->p += dt_per_frame * particle->v;
    }
}

//moderalife123!

























