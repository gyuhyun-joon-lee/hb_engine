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

enum CollisionVolumeType
{
    CollisionVolumeType_Null,
    CollisionVolumeType_Cube,
    CollisionVolumeType_Sphere,
};

struct CollisionVolumeCube
{
    CollisionVolumeType type;

    v3 offset;
    v3 half_dim;
};

struct CollisionVolumeSphere
{
    CollisionVolumeType type;

    v3 offset;
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
