#include "hb_pbd.h"

internal void
start_particle_allocation_from_pool(PBDParticlePool *pool, PBDParticleGroup *group)
{
    // TODO(gh) The original algorithm uses Quat(A) / |Quat(A)|,
    // which is very confusing because A is a 3x3 matrix, 
    // and even if A implies an object and not a matrix, 
    // we aren't storing the orientation.
    group->shape_match_quat = Quatd(1, 0, 0, 0);
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
                            v3d p, v3d v, f32 r, f32 inv_mass, i32 phase = 0)
{
    PBDParticle *particle = pool->particles + pool->count++;
    assert(pool->count <= array_count(pool->particles));

    particle->p = p;
    particle->v = v;
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

#define collision_epsilon -1.0e-6

struct CollisionSolution
{
    b32 collided;
    v3d offset0;
    v3d offset1;

    v3d contact_normal;
    f64 penetration_depth;
};

// stiffness_epsilon = inv_stiffness/square(sub_dt);
internal void
solve_collision_constraint(CollisionSolution *solution,
                            CollisionConstraint *c,
                            v3d *p0, v3d *p1) 
{
    PBDParticle *particle0 = c->particle0;
    PBDParticle *particle1 = c->particle1;

    // TODO(gh) Since this constraint is only generated when 
    // at least one of them as finite mass anyway, maybe there's no reason
    // for this checking?
    if(particle0->inv_mass + particle1->inv_mass != 0.0f)
    {
        v3d delta = *p0 - *p1;
        f64 delta_length = length(delta);

        f64 rest_length = (f64)(particle0->r + particle1->r);
        f64 C = delta_length - rest_length;
        if(C < collision_epsilon)
        {
            // Gradient of the constraint for each particles that were invovled in the constraint
            // So this is actually gradient(C(xi))
            v3d gradient0 = normalize(delta);
            v3d gradient1 = -gradient0;

            f64 lagrange_multiplier = 
                -C / (particle0->inv_mass + particle1->inv_mass);

            // NOTE(gh) delta(xi) = lagrange_multiplier*inv_mass*gradient(xi);
            // inv_mass of the particles are involved
            // so that the linear momentum is conserved(otherwise, it might produce the 'ghost force')
            solution->collided = true;
            solution->offset0 = lagrange_multiplier*(f64)particle0->inv_mass*gradient0;
            solution->offset1 = lagrange_multiplier*(f64)particle1->inv_mass*gradient1;

            solution->contact_normal = gradient0;
            // TODO(gh) C or -C
            solution->penetration_depth = abs_f64(C);
        }
    }
}

struct EnvironmentSolution
{
    v3d offset;
};

internal void
solve_environment_constraint(EnvironmentSolution *solution,
                             EnvironmentConstraint *c,
                             v3d *p)
{
    // TODO(gh) Since this constraint is only generated when 
    // the particle has finite mass anyway, maybe there's no reason
    // for this checking?
    if(c->particle->inv_mass != 0.0f)
    {
        f64 d = dot(c->n, *p);
        f64 C = d - c->d - c->particle->r;
        if(C < collision_epsilon)
        {
            // NOTE(gh) For environmental collision, we don't need the inv_mass thing
            f64 lagrange_multiplier = -C;

            // NOTE(gh) delta(xi) = lagrange_multiplier*inv_mass*gradient(xi);
            solution->offset = lagrange_multiplier*(c->n);
        }
    }
}

internal m3x3d
get_shape_matching_linear_deformation_rotation_matrix(PBDParticleGroup *group)
{
    v3d com = get_com_of_particle_group(group);

    m3x3d linear_Apq = M3x3d(); 
    for(u32 particle_index = 0;
            particle_index < group->count;
            ++particle_index)
    {
        PBDParticle *particle = group->particles + particle_index;

        v3d offset = particle->p - com;
        linear_Apq.rows[0] += offset.x * particle->initial_offset_from_com;
        linear_Apq.rows[1] += offset.y * particle->initial_offset_from_com;
        linear_Apq.rows[2] += offset.z * particle->initial_offset_from_com;
    }
    // NOTE(gh) Note that R should be retrieved from Apq, not from full A.
    group->shape_match_quat = 
        extract_rotation_from_polar_decomposition(&linear_Apq, &group->shape_match_quat, 32);
    m3x3d R = 
        orientation_quatd_to_m3x3d(group->shape_match_quat);

    m3x3d linear_A = linear_Apq * group->linear_inv_Aqq;
    // Volume preservation
    linear_A = (1.0f/cbrt(get_determinant(linear_A))) * linear_A;

    // R is the matrix that we were using for the rigid body deformation,
    // which means that it will try to recover in a 'rigid body' way.
    f64 linear_deformation_coefficient = 0.9998;
    m3x3d result = linear_deformation_coefficient*linear_A + (1.0-linear_deformation_coefficient)*R;

    return result;
}














