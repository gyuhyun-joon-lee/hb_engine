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

// TODO(gh) Is there any way to get the COM without any division,
// maybe cleverly using the inverse mass?
/*
   NOTE(gh)
   COM = m0*x0 + m1*x1 ... mn*xn / (m0 + m1 + ... mn)
*/
internal v3d
get_com_of_particle_group(PBDParticleGroup *group)

{
    v3d result = V3d();
    f64 total_mass = 0.0;
    for(u32 particle_index = 0;
            particle_index < group->count;
            ++particle_index)
    {
        PBDParticle *particle = group->particles + particle_index;
        f64 mass = 1/particle->inv_mass;
        total_mass += mass;
        assert(particle->inv_mass != 0.0);

        result += mass * particle->p;
    }
    result /= total_mass;

    return result;
}

// stiffness_epsilon = inv_stiffness/square(sub_dt);
internal void
solve_distance_constraint(PBDParticle *particle0, PBDParticle *particle1,
                          v3d delta, f64 C, f64 stiffness_epsilon)
{
    // Gradient of the constraint for each particles that were invovled in the constraint
    // So this is actually gradient(C(xi))
    v3d gradient0 = normalize(delta);
    v3d gradient1 = -gradient0;

    f64 lagrange_multiplier = 
        -C / (particle0->inv_mass + particle1->inv_mass + stiffness_epsilon);

    if(lagrange_multiplier != 0)
    {
        // NOTE(gh) delta(xi) = lagrange_multiplier*inv_mass*gradient(xi);
        // inv_mass of the particles are involved
        // so that the linear momentum is conserved(otherwise, it might produce the 'ghost force')
        v3d offset0 = lagrange_multiplier*(f64)particle0->inv_mass*gradient0;
        v3d offset1 = lagrange_multiplier*(f64)particle1->inv_mass*gradient1;
        particle0->p += offset0;
        particle1->p += offset1;
    }
}

                    // TODO(gh) Using the shape matching constraint instead of distance constraint,
                    // can we just remove distance constraint at all?
#if 0
                    for(u32 constraint_index = 0;
                            constraint_index < group->distance_constraint_count;
                            ++constraint_index)
                    {
                        DistanceConstraint *c = group->distance_constraints + constraint_index;
                        PBDParticle *particle0 = group->particles + c->index0;
                        PBDParticle *particle1 = group->particles + c->index1;

                        if(particle0->inv_mass + particle1->inv_mass != 0.0f)
                        {
                            v3d delta = particle0->p - particle1->p;
                            f64 delta_length = length(delta);

                            f64 C = delta_length - c->rest_length;

                            if(C < 0.0)
                            {
                                f64 stiffness_epsilon = (f64)group->inv_distance_stiffness/sub_dt_square;
                                solve_distance_constraint(particle0, particle1, delta, C, stiffness_epsilon);
                            }
                        }
                    }
#endif












