#ifndef HB_COLLISION_H
#define HB_COLLISION_H

struct RigidBody;

// TODO(gh) Would it be beneficial(or is it even possible) for us if we define
// contact point : deepest penetration point
// collision normal : direction of where the object should go to remove interpenetration at all
// penetration depth : how much we should go along the collision normal to remove interpenetration at all
struct ContactData
{
    v3 point; // Any point in contact 'region', so no assumption should be made about the exact location
    v3 normal; // World space, should be normalized
    f32 depth; // positive means no contact, negative means inter-penetration

    f32 collision_restitution; // How much bounce is in the collision
    f32 friction;

    RigidBody *host; // contact normal indicates the 'bounce direction' of this object
    RigidBody *test; // should use -1 * contact normal
};

struct ContactGroup
{
    ContactData *data;
    u32 count;
    u32 max_count;

};

enum CollisionVolumeType
{
    CollisionVolumeType_Null,
    CollisionVolumeType_HalfPlane,
    CollisionVolumeType_Sphere,
};

/*
   NOTE(gh) For now, we will assume that one rigidbody can hold
   only one collision volume, and all the collision volumes are 
   centered at the rigidbody COM.

   For example, if the rigid body was using collision volume sphere,
   that would mean the center of the sphere == COM
*/

// NOTE(gh) plane equation : normal * p = d
// Half plane is treated as if the opposite normal side == solid
struct CollisionVolumeHalfPlane
{
    // NOTE(gh) Type should always be at the start of the struct
    CollisionVolumeType type;

    v3 normal;
    // d would be re-caculated based on the entity position & rigid body COM
};

struct CollisionVolumeSphere
{
    // NOTE(gh) Type should always be at the start of the struct
    CollisionVolumeType type;

    v3 center;
    f32 r;
};

#endif
