#ifndef HB_COLLISION_H
#define HB_COLLISION_H

// TODO(gh) Would it be beneficial(or is it even possible) for us if we define
// contact point : deepest penetration point
// collision normal : direction of where the object should go to remove interpenetration at all
// penetration depth : how much we should go along the collision normal to remove interpenetration at all
struct ContactData
{
    // Different collision detection methods might produce different contact points,
    // but in most cases we can use them as-is
    v3 contact_point;  
    v3 collision_normal; // Should be normalized
    f32 penetration_depth;

    f32 collision_restitution; // How much bounce is in the collision
    f32 friction;
};

// NOTE(gh) Similar to the rendergroup, this will be 'cleared' 
// at the start of the frame
struct ContactGroup
{
    ContactData *contact_data;
    u32 contact_data_count;
    u32 contact_data_used;
};

/* TODO(gh) Rename these as 'entries' for
   collision volume?
*/
// NOTE(gh) plane equation : normal * p = d
// Half plane is treated as if the opposite normal side == solid
struct CollisionHalfPlane
{
    v3 normal;
    f32 d;
};

struct CollisionSphere
{
    v3 center;
    f32 r;
};

#endif
