#ifndef HB_RIGIDBODY_H
#define HB_RIGIDBODY_H

struct Entity;

struct RigidBody
{
    // TODO(gh) Not thrilled about this backward pointer
    Entity *entity;

    void *collision_volume;

    f32 inverse_mass;

    /* NOTE(gh) 
       Linear attributes
    */
    v3 center_of_mass_offset; // offset of the center of mass from the entity position
    v3 linear_velocity;
    f32 linear_damping;

    /* NOTE(gh) 
       angular attributes
    */
    // NOTE(gh) Orientation isn't the 'direction' of the object,
    // This is more like how to 'rotate' any point on object
    // which is why this is not a pure quaternion(which represents vector
    // in quaternion space)
    quat orientation; // [cos(t/2), x*sin(t/2), y*sin(t/2), z*sin(t/2)] where x, y, z is the normalized axis
    v3 angular_velocity;
    // The original inertia tensor Ia = sum(mi * d^2)
    // where mi is the mass of the particle i and
    // d is the distance of particle i from the axis a
    // However, for the rigid body, 
    // we can get away with the pre-defined inertia tensor that looks like
    // |Ix -Ixy -Ixz|
    // |-Ixy Iy -Iyz|
    // |-Ixz -Iyz Iz|
    m3x3 inverse_inertia_tensor; 
    f32 angular_damping;

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
    v3 force; // Literally the net force, might include torque inside it. World space
    v3 torque; // World space
};

#endif
