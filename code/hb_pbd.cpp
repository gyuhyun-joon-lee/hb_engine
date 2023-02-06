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
