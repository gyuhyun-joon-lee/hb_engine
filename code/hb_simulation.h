/*
 * Written by Gyuhyun Lee
 */

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

#endif
