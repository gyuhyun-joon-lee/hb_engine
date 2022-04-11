#ifndef MEKA_SIMULATION_H
#define MEKA_SIMULATION_H

// TODO(joon) collision volume & group
struct AABB
{
    // _not_ relative to the entity p,
    // each collision volume can have seperate p and dp
    v3 p;// center
    v3 dp;
    v3 half_dim;

    // TODO(joon) this is not a correct place to put mass
    f32 inv_mass;
};

// NOTE(joon) used inside mass aggregation physics
struct MassParticle
{
    f32 inv_mass;
    v3 p; // this is in world pos, and not relative to the entity p
    v3 dp; // velocity
    
    // NOTE(joon) this is where we accumulate the force while interacting with 
    // other particles(i.e elastic force)
    // IMPORTANT(joon) Should be cleared to 0 every frame!
    v3 this_frame_force;
    v3 this_frame_ddp;
};

struct ParticleFace
{
    // NOTE(joon) these are the indices to the particles, in counter clockwise order.
    u32 ID_0;
    u32 ID_1;
    u32 ID_2;

    // NOTE(joon) outward normal
    v3 normal;
};

// TODO(joon) we need a fast way to get all of the connections for certain particle. Maybe hashing?
struct PiecewiseMassParticleConnection
{
    u32 ID_0;
    u32 ID_1;

    // TODO(joon) we can also save elastic value here, if we want to have different spring for each connection.

    f32 rest_length;
};

struct MassAgg
{
    // TODO(joon) change this so that it can contain any number of particles
    MassParticle *particles;
    u32 particle_count;

    ParticleFace *faces;
    u32 face_count;

    PiecewiseMassParticleConnection *connections;
    u32 connection_count;

    // NOTE(joon) used in hooks law
    f32 elastic_value;
};

#endif
