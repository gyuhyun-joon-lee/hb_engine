#include "hb_pbd.h"

internal PBDParticleGroup
initialize_pbd_particle_group(PBDParticlePool *pool, u32 desired_count)
{
    PBDParticleGroup result = {};

    u32 max_particle_count = array_count(pool->particles);

    if(pool->count + desired_count <= max_particle_count)
    {
        result.start_index = pool->count;
        pool->count += desired_count;
        result.one_past_end_index = pool->count;
    }
    else
    {
        // NOTE(gh) We ran out of the particles
        assert(0);
    }

    return result;
}

// NOTE(gh) Projecting based on position 0,
// which means the result(d_position) will be added to the 0th particle position
// and subtracted to the 1th particle position
internal v3
project_collision_constraint(v3 delta, f32 distance_between, f32 constraint_value)
{
    // NOTE(gh) -1*delta makes sense here, because collision constraint should only be projected
    // when it's a negative value, which means that 0th particle will move in (0th - 1th) direction
    v3 result = (constraint_value/(-2.0f*distance_between)) * delta;

    return result;
}

#if 0
internal f32
get_collision_constraint_value(CollisionConstraint *constraint)
{
}

internal f32
get_environment_constraint_value()
{
    if(dot(floor_normal, particle->proposed_p) - floor_d - particle->r < 0)
}
#endif

