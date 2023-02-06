#ifndef HB_PBD_H
#define HB_PBD_H

/*
    NOTE(gh) Some equations involved in PBD
    lagrange multiplier(lambda) = C(p) / sum(abs(gradient))
*/

struct PBDParticle
{
    v3 position;
    v3 velocity;

    //  TODO(gh) For now, all particles have identical size(radius) to avoid clipping,
    // but there might be workaround for this!
    f32 radius;

    f32 inv_mass;

    // NOTE(gh) Used for grouping particles. i.e particles in the same object would have the same phase, 
    // preventing them from colliding each other
    i32 phase; 

    /*
        NOTE(gh) Temporary varaibles, should be cleared to 0 each frame
    */
    // TODO(gh) Might not be necessary(i.e don't store velocity, and get it implicitly each frame?)
    v3 proposed_position; 
    v3 d_position_sum;
    u32 constraint_hit_count;
};

// Every particle group uses the same particle 'pool',
// and each particle can be attrived using the index
// (from start_index to one_past_end_index-1)
struct PBDParticleGroup
{
    // TODO(gh) Used for entity flag, but can we remove this?
    u32 start_index;
    u32 one_past_end_index;
};

struct PBDParticlePool
{
    // TODO(gh) Probably not a good idea...
    PBDParticle particles[4096];
    u32 particle_count;
};

// TODO(gh) Can only gather certain amount of particle groups
struct GatheredPBDParticleGroups
{
    PBDParticleGroup groups[256];
    u32 count;
};

struct FixedPositionConstraint
{
    u32 index;
    v3 fixed_position; // world space
};

/*
    NOTE(gh) Collision Constraint between two particles
    C(x0, x1) = |x0 −x1|−(r0 +r1) ≥ 0 
*/
struct CollisionConstraint
{
    u32 index0;
    u32 index1;
    // r0 and r1 can be retrieved from the particle directly
};

/*
    NOTE(gh) Environment collision constraint between one particle and the environment
    C(x) = dot(plane_normal, x) − (plane_d - radius) ≥ 0
*/
struct EnvironmentCollisionConstraint
{
    u32 particle_index;

    v3 plane_normal;
    f32 plane_d;

    f32 radius;
};

/*
    NOTE(gh) Distance constraint between two particles
    C(x0, x1) = ;
*/
struct DistanceConstraint
{
    u32 particle_index0;
    u32 particle_index1;

    f32 distance;
};

// NOTE(gh) Pre Stablization.
// If the convergence was not reached due to time constraint on the previous time step,
// the initial condition when we start a new time step might be wrong, and because the velocity is implicitly calculated
// from two successive timesteps, this might cause the particle to pop.
// For example, let's say the particle was interpenetrating the ground with no velocity at time step 0.
// On time step 1, the physics engine will detect the interpenetration and move the particle up on the ground,
// and update the velocity to face upwards, because the new position is higher than the previous position
// (kinectic energy added from nowhere to the object).
// To mitigate this, pre-stabilize the scene by solving the collision constraints for the _current position_
// right before we move onto the the main constraint solver, and then updating both the current position &
// proposed position.

/*
    NOTE(gh) Complete steps for position-based rigid body
    Unified Particle Physics for Real-Time Applications(https://mmacklin.com/uppfrta_preprint.pdf)
    Position Based Dynamics by Pieterjan Bartels(https://pbartels.gitlab.io/portfolio/projects/PBD/)

    for(each particle i)
    {
        particle[i].velocity += dt*net_force(i)/m; // TODO(gh) Double check if we have to divide net force by mass
        particle[i].proposed_position = particle[i].position + dt*particle[i].velocity;
    }

    for(each particle i)
    {
        find colliding particles using the proposed position; 
        find Environment contacts; 
        generate collision constraints;
    }

    // NOTE(gh) pre-stabilization. See the description above for more info!
    for(solver iterations)
    {
        for(each particle i)
        {
            particle[i].d_position_sum = 0;
            particle[i].count = 0;
        }

        // NOTE(gh) Only process the collision constraints for pre-stabilization!
        for(each collision constraint C) 
        {
            for(each particle i affected by the constraint C)
            {
                solve constraint for i, using the _current position_ instead of the proposed position

                particle[i].d_position_sum += result;
                particle[i].constraint_count++;
            }
        }

        for(each particle i)
        {
            // Update both the current position & proposed position after the pre-stabilization
            particle[i].position +=  weight * particle[i].d_position_sum/particle[i].constraint_count;
            particle[i].proposed_position +=  weight * particle[i].d_position_sum/particle[i].constraint_count;
        }
    } // pre-stabilization end

    for(solver iterations) // TODO(gh) Test 5 times first
    {
        for(each constraint group)
        {
            for(each particle i)
            {
                // NOTE(gh) Under relaxation,
                // all constraints are processed in parallel, 
                // and we are going to average it to apply it for the next position.
                particle[i].d_position_sum = 0;
                particle[i].constraint_count = 0;
            }

            for(each constraint C)
            {
                // TODO(gh) Maybe we can pre-calculate lambda(lagrange multiplier) here,
                // because it is particle-independant
                f32 lagrange_multiplier = pre_calculate();
                for(each particle i affected by the constraint C)
                {
                    solve constraint for i using the proposed position;
                    particle[i].d_position_sum += result;
                    particle[i].count++;
                }
            }

            for(each particle i)
            {
                particle[i].proposed_position += weight * (particle[i].d_position_sum / particle[i].count);
            }
        }
    }

    for(each particle i)
    {
        v3 d_position = particle[i].proposed_position - particle[i].position;
        particle[i].velocity = (d_position)/dt;

        // NOTE(gh) particle sleeping, prevent micro stutter
        if(length_square(d_position) > epsilon)
        {
            particle[i].position = particle[i].proposed_position;
        }
    }
*/
















#endif
