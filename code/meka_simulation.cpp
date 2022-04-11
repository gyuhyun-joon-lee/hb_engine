#include "meka_simulation.h"

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

internal AABB
init_aabb(v3 center, v3 half_dim, f32 inv_mass)
{
    AABB result = {};

    result.p = center;
    result.half_dim = half_dim;
    result.inv_mass = inv_mass;

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
add_piecewise_mass_particle_connection(MemoryArena *arena, MassAgg *mass_agg, u32 ID_0, u32 ID_1)
{
    // TODO(joon) These allocations should always happen sequentially!
    PiecewiseMassParticleConnection *connection = push_struct(arena, PiecewiseMassParticleConnection);
    if(!mass_agg->connections)
    {
        mass_agg->connections = connection;
    }
    else
    {
        assert(connection == mass_agg->connections + mass_agg->connection_count);
    }

    connection->ID_0 = ID_0;
    connection->ID_1 = ID_1;
    connection->rest_length = length(mass_agg->particles[ID_0].p -
                                    mass_agg->particles[ID_1].p);

    mass_agg->connection_count++;
}

internal void
add_particle_face(MemoryArena *arena, MassAgg *mass_agg, u32 ID_0, u32 ID_1, u32 ID_2)
{
    // TODO(joon) These allocations should always happen sequentially!
    ParticleFace *face = push_struct(arena, ParticleFace);
    if(!mass_agg->faces)
    {
        mass_agg->faces = face;
    }
    else
    {
        assert(face == mass_agg->faces + mass_agg->face_count);
    }
    face->ID_0 = ID_0;
    face->ID_1 = ID_1;
    face->ID_2 = ID_2;

    face->normal = cross(mass_agg->particles[ID_2].p - mass_agg->particles[ID_1].p,
                          mass_agg->particles[ID_0].p - mass_agg->particles[ID_1].p);

    mass_agg->face_count++;
}

// TODO(joon) pass inv_mass?
// TODO(joon) pass pointer?
internal MassAgg
init_flat_triangle_mass_agg(MemoryArena *arena, f32 total_mass, f32 elastic_value)
{
    MassAgg result = {};

    result.particle_count = 4;
    result.particles = push_array(arena, MassParticle, result.particle_count);
    f32 particle_mass = total_mass / result.particle_count;

    result.particles[0].p = V3(1, -1.0f, 5.0f);
    result.particles[1].p = V3(-1, -1.0f, 5.0f);
    result.particles[2].p = V3(0, 2.0f, 5.0f);
    result.particles[3].p = V3(0, 0.0f, 7.0f);

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
    add_piecewise_mass_particle_connection(arena, &result, 2, 0); 

    add_piecewise_mass_particle_connection(arena, &result, 3, 1);
    add_piecewise_mass_particle_connection(arena, &result, 3, 2); 
    add_piecewise_mass_particle_connection(arena, &result, 3, 0); 

    return result;
}

// TODO(joon) pass inv_mass?
// TODO(joon) pass pointer?
internal MassAgg
init_cube_mass_agg(MemoryArena *arena, v3 center, v3 dim, f32 total_mass, f32 elastic_value)
{
    v3 half_dim = 0.5f*dim;
    MassAgg result = {};

    result.particle_count = 16;
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

    result.particles[8].p = center + 0.5f * V3(half_dim.x, -half_dim.y, -half_dim.z);
    result.particles[9].p = center + 0.5f * V3(half_dim.x, half_dim.y, -half_dim.z);
    result.particles[10].p = center + 0.5f * V3(-half_dim.x, half_dim.y, -half_dim.z);
    result.particles[11].p = center + 0.5f * V3(-half_dim.x, -half_dim.y, -half_dim.z);

    result.particles[12].p = center + 0.5f * V3(half_dim.x, -half_dim.y, half_dim.z);
    result.particles[13].p = center + 0.5f * V3(half_dim.x, half_dim.y, half_dim.z);
    result.particles[14].p = center + 0.5f * V3(-half_dim.x, half_dim.y, half_dim.z);
    result.particles[15].p = center + 0.5f * V3(-half_dim.x, -half_dim.y, half_dim.z);

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

    // NOTE(joon) inner edge connections
    add_piecewise_mass_particle_connection(arena, &result, 8, 9);
    add_piecewise_mass_particle_connection(arena, &result, 9, 10); 
    add_piecewise_mass_particle_connection(arena, &result, 10, 11); 
    add_piecewise_mass_particle_connection(arena, &result, 11, 8); 

    add_piecewise_mass_particle_connection(arena, &result, 12, 13); 
    add_piecewise_mass_particle_connection(arena, &result, 13, 14); 
    add_piecewise_mass_particle_connection(arena, &result, 14, 15); 
    add_piecewise_mass_particle_connection(arena, &result, 15, 12); 

    add_piecewise_mass_particle_connection(arena, &result, 8, 12); 
    add_piecewise_mass_particle_connection(arena, &result, 9, 13); 
    add_piecewise_mass_particle_connection(arena, &result, 10, 14); 
    add_piecewise_mass_particle_connection(arena, &result, 11, 15); 

    // NOTE(joon) Outer model and inner model connection
    add_piecewise_mass_particle_connection(arena, &result, 0, 8); 
    add_piecewise_mass_particle_connection(arena, &result, 1, 9); 
    add_piecewise_mass_particle_connection(arena, &result, 2, 10); 
    add_piecewise_mass_particle_connection(arena, &result, 3, 11); 
    add_piecewise_mass_particle_connection(arena, &result, 4, 12); 
    add_piecewise_mass_particle_connection(arena, &result, 5, 13); 
    add_piecewise_mass_particle_connection(arena, &result, 6, 14); 
    add_piecewise_mass_particle_connection(arena, &result, 7, 15); 

    return result;
}

internal MassAgg
init_mass_agg_from_mesh(MemoryArena *arena, v3 center, v3 *vertices, u32 vertex_count, u32 *indices, u32 index_count,
                                    f32 total_mass, f32 elastic_value)
{
    MassAgg result = {};

    result.elastic_value = elastic_value;
    result.particle_count = vertex_count;
    result.particles = push_array(arena, MassParticle, result.particle_count);

    f32 each_particle_mass = total_mass / result.particle_count;
    for(u32 particle_index = 0;
            particle_index < result.particle_count;
            ++particle_index)
    {
        MassParticle *particle = result.particles + particle_index;
        *particle = {};
        // TODO(joon) We don't need safe ratio here, we are not even passing the inv mass 
        particle->inv_mass = safe_ratio(1.0f, each_particle_mass);
        particle->p = center + 0.005f * vertices[particle_index];
        particle->dp = V3(0, 0, 0);
    }

    // NOTE(joon) add faces 
    assert(index_count%3 == 0);
    u32 face_count =  index_count / 3;
    for(u32 face_index = 0;
            face_index < face_count; 
            ++face_index)
    {
        add_particle_face(arena, &result, 
                          indices[3 * face_index], 
                          indices[3 * face_index + 1], 
                          indices[3 * face_index + 2]);
    }
    
    for(u32 face_index = 0;
            face_index < face_count; 
            ++face_index)
    {
        ParticleFace *face = result.faces + face_index;

        b32 connection_0_1_exist = false;
        b32 connection_1_2_exist = false;
        b32 connection_2_0_exist = false;

        for(u32 connection_index = 0;
                connection_index < result.connection_count;
                ++connection_index)
        {
            PiecewiseMassParticleConnection *connection = result.connections + connection_index;
            if((connection->ID_0 == face->ID_0 && connection->ID_1 == face->ID_1) ||
                (connection->ID_0 == face->ID_1 && connection->ID_1 == face->ID_0))
            {
                connection_0_1_exist = true;
            }
            if((connection->ID_0 == face->ID_1 && connection->ID_1 == face->ID_2) ||
                (connection->ID_0 == face->ID_2 && connection->ID_1 == face->ID_1))
            {
                connection_1_2_exist = true;
            }
            if((connection->ID_0 == face->ID_0 && connection->ID_1 == face->ID_2) ||
                (connection->ID_0 == face->ID_2 && connection->ID_1 == face->ID_0))
            {
                connection_2_0_exist = true;
            }
        }

        if(!connection_0_1_exist)
        {
            add_piecewise_mass_particle_connection(arena, &result, face->ID_0, face->ID_1);
        }
        if(!connection_1_2_exist)
        {
            add_piecewise_mass_particle_connection(arena, &result, face->ID_1, face->ID_2);
        }
        if(!connection_2_0_exist)
        {
            add_piecewise_mass_particle_connection(arena, &result, face->ID_2, face->ID_0);
        }
    }

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

    for(u32 connection_index = 0;
            connection_index < mass_agg->connection_count;
            ++connection_index)
    {
        PiecewiseMassParticleConnection *connection = mass_agg->connections + connection_index;

        MassParticle *particle_0 = mass_agg->particles + connection->ID_0;
        MassParticle *particle_1 = mass_agg->particles + connection->ID_1;
        v3 delta = particle_1->p - particle_0->p;
        f32 delta_length = length(delta);
        f32 length_diff = delta_length - connection->rest_length;

        // TODO(joon) We do change both of the connected particles' positions, 
        // but this all happens sequentially.. can we do something better?
        if(abs(length_diff) > 0.0f)
        {
            v3 elastic_force = mass_agg->elastic_value * length_diff * (delta/delta_length); 
            if(length(elastic_force) >= 10.0f)
            {
                int breakhere = 0;
            }
            particle_0->this_frame_force += elastic_force; 
            particle_1->this_frame_force -= elastic_force;
        }
    }

    // NOTE(joon) first, apply gravity to start collision
    for(u32 particle_index = 0;
            particle_index < mass_agg->particle_count;
            ++particle_index)
    {
        MassParticle *particle = mass_agg->particles + particle_index;

        if(magic && particle_index == 0)
        {
            particle->this_frame_force += V3(10, 0, 10);
        }
        
        // NOTE(joon) a = F/m;
        v3 ddp = particle->inv_mass * particle->this_frame_force;
        // NOTE(joon) apply gravity
        ddp.z -= 9.8f;
        particle->dp += dt_per_frame * ddp;
#if 1
#endif
        v3 dp = dt_per_frame * particle->dp;

        if(is_entity_flag_set(entity, Entity_Flag_Collides))
        {
            for(u32 iter = 0;
                    iter < 4;
                    ++iter)
            {
                if(length(dp) > 0.0f)
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
                        //v3 ddp_by_reaction_force = dot(-ddp, hit_normal) * hit_normal;
                        //particle->dp += dt_per_frame * ddp_by_reaction_force;
                        // TODO(joon) also need to update the hit entity reaction force

                        // NOTE(joon) Even if the name is 'velocity', this is a scalar value
                        // positive value = two entities are seperating
                        // negative value = two entities are closing
                        f32 seperating_v = dot(particle->dp - hit_entity->aabb.dp, hit_normal);
                        
                        if(seperating_v < 0.0f)
                        {
                            // TODO(joon) This should differ from the material of the two entities
                            // that are colliding
                            f32 restitution_c = 0.9999f; 
                            f32 new_seperating_v = -restitution_c * seperating_v;

#if 1
                            v3 acc_caused_v = ddp;
                            f32 acc_caused_seperating_v = dt_per_frame * dot(hit_normal, acc_caused_v);

                            if(acc_caused_seperating_v < 0.0f)
                            {
                                new_seperating_v += restitution_c * acc_caused_seperating_v;
                                if(new_seperating_v < 0.0f)
                                {
                                    new_seperating_v = 0.0f;
                                }
                            }
#endif

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

                        v3 entity_delta_to_move = (min_t - 0.0001f) * dp;
                        particle->p += entity_delta_to_move;

                        dp -= entity_delta_to_move;
                        dp = dp - 2.0f * dot(hit_normal, dp) * hit_normal;
                        // TODO(joon) Do we also need to do something with the velocity here?
                    }
                    else
                    {
                        // NOTE(joon) Did not hit anything
                        particle->p += dp;
                        dp = V3(0, 0, 0);
                    }
                }
            }
        }
        else
        {
            // NOTE(joon) This entity does not collide with anything
            particle->p += dp;
        }

        // NOTE(joon) clear single frame only variables
        particle->this_frame_force = V3();

        // TODO(joon) drag should vary from entity to entity
        // TODO(joon) should we do this before testing the collisions, or after collisions
        f32 drag = 0.97f;
        particle->dp *= drag; 

        // TODO(joon) resolve interpenetration
        // To do that, we need to get the penetration depth along the contact normal.
        // p.d > 0 -> penetration, need to resolve
        // p.d = 0 -> touching each other
        // p.d < 0 -> no penetration
        // remember that resolving the interpenetration should happen only once per particle
        // after the collision iteration.

        // Also, based on the type of two entities, the distance that we should move the entities can differ.
        // i.e. entity - floor -> entity should move 
        // entity A - entity B -> (mB / (mA + mB)) * p.d, (mA / (mA + mB)) * p.d.
        // Note that the distance is based on the _opponent_'s mass, and direction should be opposite.
    }
}

