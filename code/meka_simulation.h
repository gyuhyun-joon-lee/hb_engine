#ifndef MEKA_SIMULATION_H
#define MEKA_SIMULATION_H

// TODO(joon) collision volume & group
struct AABB
{
    v3 center; 
    v3 half_dim;
};

struct RigidBody
{
    // constant
    f32 mass;
    m3 I_body;
    m3 inv_I_body;

    // state
    v3 pos;

    m3 rotation; // TODO(joon) replace this with quarternions
    v3 P; // linear momentum, P = m * v, P' = F = m * a
    v3 L; // angular momentum, L = I * w

    // derived
    m3 inv_I; // I = R * I_body * RT, inv_I = R * inv_I_body * RT
    v3 v; // linear velocity
    v3 w; // angular velocity

    // computed
    v3 F; // force
    v3 T; // torque
};

// NOTE(joon) used inside mass aggregation physics
struct MassParticle
{
    f32 inv_mass;
    v3 v;
    v3 p; // this is in world pos, and not relative to the entity p
};

// TODO(joon) we need a fast way to get all of the connections for certain particle.
struct PiecewiseMassParticleConnection
{
    u32 particle_index_0;
    u32 particle_index_1;

    // TODO(joon) we can also save elastic value here, if we want to have different spring for each connection.
};

// TODO(joon) how do we represent a 'connection' between two particles?
struct MassAgg
{
    // TODO(joon) change this so that it can contain any number of particles
    MassParticle *particles;
    u32 particle_count;

    PiecewiseMassParticleConnection *connections; // particle connections
    u32 connection_count;

    // NOTE(joon) two particles will act like a soft spring, and this is a value 
    // that is used in hook's law
    f32 elastic_value;
};

#endif
