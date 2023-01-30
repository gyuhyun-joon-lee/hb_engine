#ifndef HB_COLLISION_H
#define HB_COLLISION_H

// TODO(gh) Would it be beneficial(or is it even possible) for us if we define
// contact point : deepest penetration point
// collision normal : direction of where the object should go to remove interpenetration at all
// penetration depth : how much we should go along the collision normal to remove interpenetration at all
struct ContactData
{
    // TODO(gh) I need to figure out which space are these in
    // Different collision detection methods might produce different contact points,
    // but in most cases we can use them as-is
    v3 point;  
    v3 normal; // Should be normalized
    f32 penetration_depth;

    f32 collision_restitution; // How much bounce is in the collision
    f32 friction;
};

// NOTE(gh) Similar to the rendergroup, this will be 'cleared' 
// at the start of the frame
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
