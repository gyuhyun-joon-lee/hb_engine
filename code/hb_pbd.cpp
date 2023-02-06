#include "hb_pbd.h"

internal PBDParticleGroup
initialize_pbd_particle_group(PBDParticlePool *pool, u32 desired_count)
{
    PBDParticleGroup result = {};

    u32 max_particle_count = array_count(pool->particles);

    if(pool->particle_count + desired_count <= max_particle_count)
    {
        result.start_index = pool->particle_count;
        pool->particle_count += desired_count;
        result.one_past_end_index = pool->particle_count;
    }
    else
    {
        // NOTE(gh) We ran out of the particles
        assert(0);
    }

    return result;
}

// NOTE(gh) Solving based on position 0,
// which means the result(d_position) will be added to the 0th particle position
// and subtracted to the 1th particle position
internal v3
solve_collision_constraint(v3 position0, f32 radius0, v3 position1, f32 radius1)
{
    v3 delta = position0 - position1;
    f32 distance_between = length(delta);

    f32 constraint_value = distance_between - (radius0 + radius1);

    v3 result = (constraint_value/(-2.0f*distance_between)) * delta;
    return result;
}
