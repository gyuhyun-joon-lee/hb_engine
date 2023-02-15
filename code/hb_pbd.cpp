#include "hb_pbd.h"

internal void
start_particle_allocation_from_pool(PBDParticlePool *pool, PBDParticleGroup *group)
{
    group->particles = pool->particles + pool->count;
    // Temporary store the pool count
    group->count = pool->count;
}

internal void
end_particle_allocation_from_pool(PBDParticlePool *pool, PBDParticleGroup *group)
{
    group->count = pool->count - group->count;
    assert(group->count > 0);
}

internal void
allocate_particle_from_pool(PBDParticlePool *pool, 
                            v3d p, f32 r, f32 inv_mass, i32 phase = 0)
{
    PBDParticle *particle = pool->particles + pool->count++;
    assert(pool->count <= array_count(pool->particles));

    particle->p = p;
    particle->v = V3d(0, 0, 0);
    particle->r = r;
    particle->inv_mass = inv_mass;

    // Intializing temp variables
    particle->prev_p = V3d(0, 0, 0); 
    particle->d_p_sum = V3(0, 0, 0);
    particle->constraint_hit_count = 0;
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












