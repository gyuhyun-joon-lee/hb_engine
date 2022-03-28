#include "meka_simulation.h"
/*
   NOTE(joon):
   1. v3(0, 0, 0) inside the simulation space is literally the origin of the axises. 
*/

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
    v3 rel_a_center = a->p - b->p;

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
add_piecewise_mass_particle_connection(MemoryArena *arena, MassAgg *mass_agg, u32 particle_index_0, u32 particle_index_1)
{
    PiecewiseMassParticleConnection *connection = push_struct(arena, PiecewiseMassParticleConnection);
    if(!mass_agg->connections)
    {
        mass_agg->connections = connection;
    }
    else
    {
        assert(connection == mass_agg->connections + mass_agg->connection_count);
    }

    connection->particle_index_0 = particle_index_0;
    connection->particle_index_1 = particle_index_1;
    connection->rest_length = length(mass_agg->particles[particle_index_0].p -
                                    mass_agg->particles[particle_index_1].p);

    mass_agg->connection_count++;
}

// TODO(joon) pass inv_mass?
// TODO(joon) pass pointer?
internal MassAgg
init_cube_mass_agg(MemoryArena *arena, v3 center, v3 dim, f32 total_mass, f32 elastic_value)
{
    v3 half_dim = 0.5f*dim;
    MassAgg result = {};

    result.particle_count = 9;
    result.particles = push_array(arena, MassParticle, result.particle_count);
    f32 particle_mass = total_mass / result.particle_count;

    result.particles[0].p = center + V3(half_dim.x, -half_dim.y, -half_dim.z);
    result.particles[1].p = center + V3(half_dim.x, half_dim.y, -half_dim.z);
    result.particles[2].p = center + V3(-half_dim.x, half_dim.y, -half_dim.z);
    result.particles[3].p = center + V3(-half_dim.x, -half_dim.y, -half_dim.z);

    result.particles[4].p = center + V3(half_dim.x, -half_dim.y, half_dim.z);
    result.particles[5].p = center + V3(half_dim.x, half_dim.y, half_dim.z);
    result.particles[6].p = center + V3(-half_dim.x, half_dim.y, half_dim.z);
    result.particles[7].p = center + V3(-half_dim.x, -half_dim.y, half_dim.z);

    result.particles[8].p = center;

    result.elastic_value = elastic_value;

    for(u32 particle_index = 0;
            particle_index < result.particle_count;
            ++particle_index)
    {
        MassParticle *particle = result.particles + particle_index;
        particle->inv_mass = safe_ratio(1.0f, particle_mass);
    }

    // NOTE(joon) NO MEMORY ALLOCATION SHOULD HAPPEN WHILE ADDING THE PARTICLE CONNECTIONS
    // TODO(joon) Should we pre-allocate the connections, and use them instead?

    // NOTE(joon) edge connections
    add_piecewise_mass_particle_connection(arena, &result, 0, 1);
    add_piecewise_mass_particle_connection(arena, &result, 1, 2); 
    add_piecewise_mass_particle_connection(arena, &result, 2, 3); 
    add_piecewise_mass_particle_connection(arena, &result, 3, 0); 

    add_piecewise_mass_particle_connection(arena, &result, 4, 5); 
    add_piecewise_mass_particle_connection(arena, &result, 5, 6); 
    add_piecewise_mass_particle_connection(arena, &result, 6, 7); 
    add_piecewise_mass_particle_connection(arena, &result, 7, 4); 

    add_piecewise_mass_particle_connection(arena, &result, 0, 4); 
    add_piecewise_mass_particle_connection(arena, &result, 1, 5); 
    add_piecewise_mass_particle_connection(arena, &result, 6, 2); 
    add_piecewise_mass_particle_connection(arena, &result, 7, 3); 

    // NOTE(joon) connects vertex to vertex, diagonally
    add_piecewise_mass_particle_connection(arena, &result, 0, 2); 
    add_piecewise_mass_particle_connection(arena, &result, 1, 3); 
    add_piecewise_mass_particle_connection(arena, &result, 4, 6); 
    add_piecewise_mass_particle_connection(arena, &result, 5, 7); 

    add_piecewise_mass_particle_connection(arena, &result, 1, 6); 
    add_piecewise_mass_particle_connection(arena, &result, 5, 2); 
    add_piecewise_mass_particle_connection(arena, &result, 4, 3); 
    add_piecewise_mass_particle_connection(arena, &result, 0, 7); 

    add_piecewise_mass_particle_connection(arena, &result, 1, 4); 
    add_piecewise_mass_particle_connection(arena, &result, 5, 0); 
    add_piecewise_mass_particle_connection(arena, &result, 2, 7); 
    add_piecewise_mass_particle_connection(arena, &result, 6, 3); 

    // NOTE(joon) center to each vertices
    add_piecewise_mass_particle_connection(arena, &result, 8, 0); 
    add_piecewise_mass_particle_connection(arena, &result, 8, 1); 
    add_piecewise_mass_particle_connection(arena, &result, 8, 2); 
    add_piecewise_mass_particle_connection(arena, &result, 8, 3); 
    add_piecewise_mass_particle_connection(arena, &result, 8, 4); 
    add_piecewise_mass_particle_connection(arena, &result, 8, 5); 
    add_piecewise_mass_particle_connection(arena, &result, 8, 6); 
    add_piecewise_mass_particle_connection(arena, &result, 8, 7); 

    return result;
}

// TODO(joon) make this to test against multiple flags?
internal b32
is_entity_flag_set(Entity *entity, EntityFlag flag)
{
    b32 result = false;

    if(entity->flags | flag)
    {
        result = true;
    }

    return result;
}

struct CollTestResult
{
    f32 hit_t;
    v3 hit_normal;
};

internal CollTestResult
coll_test_ray_aabb(AABB *aabb, v3 ray_origin, v3 ray_dir)
{
    CollTestResult result = {};

    v3 aabb_min = aabb->p - aabb->half_dim;
    v3 aabb_max = aabb->p + aabb->half_dim;

    v3 inv_ray_dir = V3(1.0f/ray_dir.x, 1.0f/ray_dir.y, 1.0f/ray_dir.z);
    
    v3 t_0 = hadamard(aabb_max - ray_origin, inv_ray_dir);
    v3 t_1 = hadamard(aabb_min - ray_origin, inv_ray_dir);

    v3 t_min = gather_min_elements(t_0, t_1);
    v3 t_max = gather_max_elements(t_0, t_1);

    f32 t_min_of_max = min_element(t_max);
    f32 t_max_of_min = max_element(t_min);

    if(t_max_of_min < t_min_of_max)
    {
        result.hit_t = t_max_of_min;
        // TODO(joon) proper hit normal calculation
        result.hit_normal = V3(0, 0, 1);
        //result_hit_normal = ;
    }

    return result;
}

internal void
move_mass_agg_entity(GameState *game_state, Entity *entity, f32 dt_per_frame, b32 magic)
{
    MassAgg *mass_agg = &entity->mass_agg;
    // TODO(joon) drag should vary from entity to entity
    f32 drag = 0.97f;

    // NOTE(joon) first, apply gravity to start collision
    for(u32 particle_index = 0;
            particle_index < mass_agg->particle_count;
            ++particle_index)
    {
        MassParticle *particle = mass_agg->particles + particle_index;
        
        // NOTE(joon) apply gravity
        v3 ddp = V3();
        ddp.z -= 9.8f;
        particle->dp += dt_per_frame * ddp;

        for(u32 connection_index = 0;
                connection_index < mass_agg->connection_count;
                ++connection_index)
        {
            PiecewiseMassParticleConnection *connection = mass_agg->connections + connection_index;

            MassParticle *connected_particle = 0;
            if(particle_index == connection->particle_index_0)
            {
                connected_particle = mass_agg->particles + connection->particle_index_1;
            }
            else if(particle_index == connection->particle_index_1)
            {
                connected_particle = mass_agg->particles + connection->particle_index_0;
            }

            // TODO(joon) both of the particles that are connectied will check this and try to mitigate this,
            // which might cause some problem
            if(connected_particle)
            {
                v3 delta = connected_particle->p - particle->p;
                f32 delta_length = length(delta);
                f32 length_diff = delta_length - connection->rest_length;

                // TODO(joon) We do change both of the connected particles' positions, 
                // but this all happens sequentially.. can we do something better?
                // NOTE(joon) hook's law, elastic force = k(elastic_value) * length_diff * (connected_particle_p - this_particle_p)
                v3 elastic_force = mass_agg->elastic_value * length_diff * (delta/delta_length); 
                particle->this_frame_force += elastic_force; 
                connected_particle->this_frame_force -= elastic_force;
            }
        }

#if 1
        if(magic && particle_index == 0)
        {
            particle->this_frame_force += V3(0, 0, 10);
        }
#endif

        particle->dp += dt_per_frame * particle->inv_mass * particle->this_frame_force;
        particle->dp *= drag; // TODO(joon) should we do this before testing the collisions, or after collisions

        particle->this_frame_force = V3(); // clear to 0 for the next frame

        v3 dp = dt_per_frame * particle->dp;

        if(is_entity_flag_set(entity, Entity_Flag_Collides))
        {
            Entity *hit_entity = 0;
            f32 min_t = 1.0f;
            v3 hit_normal = V3();

            for(u32 test_entity_index = 0;
                    test_entity_index < game_state->entity_count;
                    ++test_entity_index)
            {
                Entity *test_entity = game_state->entities + test_entity_index;
                if(entity != test_entity && 
                    is_entity_flag_set(test_entity, Entity_Flag_Collides))
                {
                    if(test_entity->type == Entity_Type_Floor)
                    {
                        AABB *test_aabb = &test_entity->aabb;

                        CollTestResult coll_test_result = 
                            coll_test_ray_aabb(test_aabb, particle->p, dp);

                        // TODO(joon) resting contact
                        if(coll_test_result.hit_t < min_t &&
                            coll_test_result.hit_t > 0.0f)
                        {
                            hit_entity = test_entity;
                            hit_normal = coll_test_result.hit_normal;
                            min_t = coll_test_result.hit_t;
                        }

                    }
                }
            }

            if(hit_entity)
            {
                // NOTE(joon) Even if the name is 'velocity', this is a scalar value
                // positive value = two entities are seperating
                // negative value = two entities are closing
                f32 seperating_v = dot(particle->dp - hit_entity->aabb.dp, hit_normal);
                
                if(seperating_v < 0.0f)
                {
                    // TODO(joon) This should differ from the material of the two entities
                    // that are colliding
                    f32 restitution_c = 1.0f; 
                    f32 new_seperating_v = -1.0f * restitution_c * seperating_v;

                    f32 delta_seperating_v = new_seperating_v - seperating_v;

                    // NOTE(joon) This might look a little bit weird,
                    // but this is actually true, as the impulse is propotional to
                    // the opponent's mass
                    f32 total_inv_mass = particle->inv_mass + hit_entity->aabb.inv_mass; // this gives us (m+M)/mM;

                    f32 total_impulse = delta_seperating_v / total_inv_mass; // this gives us v * Mm/(m+M) 
                    v3 impulse_per_inv_mass = total_impulse * hit_normal;

                    if(is_entity_flag_set(hit_entity, Entity_Flag_Movable))
                    {
                        particle->dp += particle->inv_mass * impulse_per_inv_mass;
                    }

                    if(is_entity_flag_set(hit_entity, Entity_Flag_Movable))
                    {
                        hit_entity->aabb.dp -= hit_entity->aabb.inv_mass * impulse_per_inv_mass;
                    }
                }
            }

            // TODO(joon) avoid epsilon?
            particle->p += (min_t - 0.001f) * dp;
        }
        else
        {
            particle->p += dp;
        }
    }
}
