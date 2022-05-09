#ifndef HB_SIMULATION_H
#define HB_SIMULATION_H

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

struct ParticleConnection
{
    u32 opponenet_ID;
    f32 rest_length;
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

    // NOTE(joon) Direct connections will be always gathered when simulating the particle.
    //u32 direct_connection_count;
    //ParticleConnection direct_connections;
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

    f32 rest_length;
};

struct MassAgg
{
    MassParticle *particles;
    u32 particle_count;

    ParticleFace *faces;
    u32 face_count;

    // TODO(joon) any way to collapse face and connection?
    PiecewiseMassParticleConnection *connections;
    u32 connection_count;

    // NOTE(joon) hooks law
    f32 elastic_value;

    // NOTE(joon) this is a imaginary sphere that will prevent certain particle to go through the 
    f32 inner_sphere_r;
};

enum CollisionVolumeType
{
    CollisionVolumeType_Null,
    CollisionVolumeType_Sphere,
    CollisionVolumeType_Cube,
};

struct CollisionVolumeHeader
{
    CollisionVolumeType type;
    v3 offset;
};

struct CollisionVolumeCube
{
    CollisionVolumeType type;
    v3 dim;
};

struct CollisionVolumeSphere
{
    CollisionVolumeType type;
    f32 r;
};

struct CollisionVolumeGroup
{
    u8 *collision_volume_buffer;
    u32 size;
    u32 used;
};

struct RigidBody
{
    f32 inv_mass;
    //f32 linear_damp;
    // NOTE(joon) updated each frame using the equation 
    // o = o + (dt*angular_v*o)/2
    quat orientation; 

    m3x3 inv_inertia_tensor; // NOTE(joon) in local space
    //f32 angular_damp;

    v3 p; // NOTE(Also works as a center of mass)
    v3 dp;

    // TODO(joon) collision volume instead!!!!
    v3 dim;
    f32 r;

    // NOTE(joon) needs to be a pure quaternion
    v3 angular_dp;

    /* NOTE(joon) derived parameters per frame */
    m4x4 transform; // derived from orientation quaternion
    m3x3 transform_inv_inertia_tensor; // derived from inv_inertia_tensor, convert to local -> multiply by inv it -> convert back to world

    /* NOTE(joon) Computed parameters per frame */
    v3 force; // world space
    v3 torque; // world space
};

#endif
