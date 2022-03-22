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

#if 0
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
#endif

internal void
add_piecewise_mass_particle_connection(MassAgg *mass_agg, u32 particle_index_0, u32 particle_index_1)
{
    PiecewiseMassParticleConnection *connection = mass_agg->connections + mass_agg->connection_count++;

    connection->particle_index_0 = particle_index_0;
    connection->particle_index_1 = particle_index_1;
}


// NOTE(joon) all the edges are the same sized
// TODO(joon) pass inv_mass?
// TODO(joon) pass pointer?
internal MassAgg
init_cube_mass_agg(MemoryArena *arena, v3 base_p, f32 dim, f32 mass, f32 elastic_value)
{
    f32 half_dim = 0.5f*dim;
    MassAgg result = {};

    result.particle_count = 8;
    result.particles = push_array(arena, MassParticle, result.particle_count);

    result.particles[0].p = base_p + V3(half_dim, -half_dim, -half_dim);
    result.particles[1].p = base_p + V3(half_dim, half_dim, -half_dim);
    result.particles[2].p = base_p + V3(-half_dim, half_dim, -half_dim);
    result.particles[3].p = base_p + V3(-half_dim, -half_dim, -half_dim);

    result.particles[4].p = base_p + V3(half_dim, -half_dim, half_dim);
    result.particles[5].p = base_p + V3(half_dim, half_dim, half_dim);
    result.particles[6].p = base_p + V3(-half_dim, half_dim, half_dim);
    result.particles[7].p = base_p + V3(-half_dim, -half_dim, half_dim);

    result.elastic_value = 1.0f;

    for(u32 particle_index = 0;
            particle_index < result.particle_count;
            ++particle_index)
    {
        MassParticle *particle = result.particles + particle_index;
        particle->inv_mass = 1.0f/mass;
    }

    u32 connection_count = 12;
    result.connections = push_array(arena, PiecewiseMassParticleConnection, connection_count);

    // TODO(joon) Should we pre-allocate the connections, and use them instead?
    add_piecewise_mass_particle_connection(&result, 0, 1); 
    add_piecewise_mass_particle_connection(&result, 1, 2); 
    add_piecewise_mass_particle_connection(&result, 2, 3); 
    add_piecewise_mass_particle_connection(&result, 3, 0); 

    add_piecewise_mass_particle_connection(&result, 4, 5); 
    add_piecewise_mass_particle_connection(&result, 5, 6); 
    add_piecewise_mass_particle_connection(&result, 6, 7); 
    add_piecewise_mass_particle_connection(&result, 7, 4); 

    add_piecewise_mass_particle_connection(&result, 0, 4); 
    add_piecewise_mass_particle_connection(&result, 1, 5); 
    add_piecewise_mass_particle_connection(&result, 6, 2); 
    add_piecewise_mass_particle_connection(&result, 7, 3); 

    assert(result.connection_count == connection_count);

    return result;
}

internal void
move_mass_agg(GameState *game_state, MassAgg *mass_agg, f32 dt_per_frame)
{
    // TODO(joon) drag should vary from entity to entity
    f32 drag = 0.9f;
    // NOTE(joon) gravity is universal for all of the particles, unless there is no collision.
    for(u32 particle_index = 0;
            particle_index < mass_agg->particle_count;
            ++particle_index)
    {
        MassParticle *particle = mass_agg->particles + particle_index;
        particle->v.z -= 9.8f * dt_per_frame;
        particle->v *= drag; // TODO(joon) should we do this before testing the collisions, or after collisions
    }

    for(u32 particle_index = 0;
            particle_index < mass_agg->particle_count;
            ++particle_index)
    {
        MassParticle *particle = mass_agg->particles + particle_index;
        v3 new_p = particle->p + dt_per_frame * particle->v;

#if 0
        for(u32 connection_index = 0;
                connection_index < mass_agg->connection_count;
                ++connection_index)
        {
            PiecewiseMassParticleConnection *connection = mass_agg->connections + connection_index;
            // TODO(joon) both of the particles that are connectied will check this and try to mitigate this,
            // which might cause some problem
            if(particle_index == connection->particle_index_0 ||
                particle_index == connection->particle_index_1)
            {
                // if(the length is bigger than the allowed length)
                // apply elastic force
            }
        }
#endif

        particle->p = new_p;
    }
}


























