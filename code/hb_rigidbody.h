#ifndef HB_RIGIDBODY_H
#define HB_RIGIDBODY_H

struct RigidBody
{
    f32 inverse_mass;
    f32 linear_damp;

    /* NOTE(gh) 
       Linear attributes
    */
    v3 position; // Center of mass
    v3 velocity;

    /* NOTE(gh) 
       angular attributes
    */
    // NOTE(gh) This isn't the 'direction' of the object,
    // This is more like how to 'rotate' any point on object
    // which is why this is not a pure quaternion(which represents vector
    // in quaternion space)
    quat orientation;
    v3 angular_velocity;
    m3x3 inverse_inertia_tensor;

    /*
       NOTE(gh)
       Derived attributes, should be updated per frame
    */
    m3x4 transform_matrix;
    m3x3 world_inverse_inertia_tensor;

    /*
        NOTE(gh)
        Accumulated attributes, should be cleared to zero at the start of the frame
    */
    v3 force; // Literally the net force, might include torque inside it
    v3 torque;
};

#endif
