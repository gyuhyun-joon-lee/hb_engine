#include "hb_rigidbody.h"

/*
   TODO(gh)
   I guess this also means that we need to convert orientation quaternion -> matrix function later for :

   local_to_world(m3x4 *transform, v3 local)
   local_to_world_vector(m3x4 *transform, v3 v)
   world_to_local(m3x4 *transform, v3 world)
   world_to_local_vector(m3x4 *transform, v3 v)
*/
internal v3
local_to_world(m3x4 *transform, v3 local)
{
    // NOTE(gh) Because the input was the coordinate instead of vector, 
    // we need to stitch 1.0f(which will handle the translation)
    v3 result = *transform * V4(local, 1.0f);

    return result;
}

internal v3
local_to_world_vector(m3x4 *transform, v3 v)
{
    // TODO(gh) Could also simplify this as w is 0
    v3 result = *transform * V4(v, 0.0f);

    return result;
}

internal v3
world_to_local(m3x4 *transform, v3 world)
{
    // Beceause local to world transformation is first rotate and then translate,
    // we can get world to local coordinates by translating and then rotating both in opposite direction(angle)

    /* NOTE(gh) This is whole operation is same as 
       transpose(rotation part) * (world - V3(transform->e03, transform->e13, transform->e23))
    */
    v3 after_opposite_translation = world - V3(transform->e[0][3], transform->e[1][3], transform->e[2][3]);

    v3 result = {};

    result.x = dot(V3(transform->e[0][0], transform->e[1][0], transform->e[2][0]), after_opposite_translation);
    result.y = dot(V3(transform->e[0][1], transform->e[1][1], transform->e[2][1]), after_opposite_translation);
    result.z = dot(V3(transform->e[0][2], transform->e[1][2], transform->e[2][2]), after_opposite_translation);

    return result;
}

internal v3
world_to_local_vector(m3x4 *transform, v3 v)
{
    v3 result = {};

    result.x = dot(V3(transform->e[0][0], transform->e[1][0], transform->e[2][0]), v);
    result.y = dot(V3(transform->e[0][1], transform->e[1][1], transform->e[2][1]), v);
    result.z = dot(V3(transform->e[0][2], transform->e[1][2], transform->e[2][2]), v);

    return result;
}

internal void
apply_force_at_world_space_point(RigidBody *rigid_body,
                                v3 force, // linear + angular, world space
                                v3 world_p)
{
}

internal void
apply_force_at_local_space_point(RigidBody *rigid_body,
                                v3 force, // linear + angular, world space
                                v3 local_p)
{
}

internal void
update_derived_rigid_body_attributes(RigidBody *rigid_body)
{
    // NOTE(gh) Quaternion has 4 DOF, but we want 3 DOF, so we normalize the quaternion (4-1=3)
    rigid_body->orientation = normalize(rigid_body->orientation);

    /*
       NOTE(gh) Update transform matrix
    */
    m3x4 *transform_matrix = &rigid_body->transform_matrix;
    quat *orientation = &rigid_body->orientation;
    transform_matrix->e[0][0] = 1 - 2.0f*(orientation->y*orientation->y + orientation->z*orientation->z);
    transform_matrix->e[0][1] = 2.0f*(orientation->x*orientation->y - orientation->s*orientation->z);
    transform_matrix->e[0][2] = 2.0f*(orientation->x*orientation->z + orientation->s*orientation->y);
    transform_matrix->e[0][3] = rigid_body->position.x;

    transform_matrix->e[1][0] = 2.0f*(orientation->x*orientation->y + orientation->s*orientation->z);
    transform_matrix->e[1][1] = 1 - 2.0f*(orientation->x*orientation->x + orientation->z*orientation->z);
    transform_matrix->e[1][2] = 2.0f*(orientation->y*orientation->z - orientation->s*orientation->x);
    transform_matrix->e[1][3] = rigid_body->position.y;

    transform_matrix->e[2][0] = 2.0f*(orientation->x*orientation->z - orientation->s*orientation->y);
    transform_matrix->e[2][1] = 2.0f*(orientation->y*orientation->z + orientation->x*orientation->s);
    transform_matrix->e[2][2] = 1 - 2.0f*(orientation->x*orientation->x + orientation->y*orientation->y);
    transform_matrix->e[2][3] = rigid_body->position.z;

    /*
       NOTE(gh) Get world space inverse inertia tensor.
       This is done with these steps(note that the transform matrix we are using here is 3x3 matrix,
       which means no translation) : 
       1. Transform to local space by multiplying by transform matrix
       2. Multiply by inverse inertia tensor(defined in local space)
       3. Transofrm back to world space by multiplying by inverse(in this case transpose, because only rotation) 
       transform matrix

       So if we were to get the angular acceleration with world space torque, the steps will be identical to : 
       1. Transform the world space torque into local space
       2. Multiply by local sapce inverse inertia tensor to get the angular acceleration(angular acc = iit x torque)
       3. Transform the angular acc back to world space
    */

    m3x3 *inverse_inertia_tensor = &rigid_body->inverse_inertia_tensor;
    f32 iit_x_transform_00 = transform_matrix->e[0][0] * inverse_inertia_tensor->e[0][0] + 
                              transform_matrix->e[0][1] * inverse_inertia_tensor->e[1][0] + 
                              transform_matrix->e[0][2] * inverse_inertia_tensor->e[2][0];

    f32 iit_x_transform_01 = transform_matrix->e[0][0] * inverse_inertia_tensor->e[0][1] + 
                              transform_matrix->e[0][1] * inverse_inertia_tensor->e[1][1] + 
                              transform_matrix->e[0][2] * inverse_inertia_tensor->e[2][1];

    f32 iit_x_transform_02 = transform_matrix->e[0][0] * inverse_inertia_tensor->e[0][2] + 
                              transform_matrix->e[0][1] * inverse_inertia_tensor->e[1][2] + 
                              transform_matrix->e[0][2] * inverse_inertia_tensor->e[2][2];

    f32 iit_x_transform_10 = transform_matrix->e[1][0] * inverse_inertia_tensor->e[0][0] + 
                              transform_matrix->e[1][1] * inverse_inertia_tensor->e[1][0] + 
                              transform_matrix->e[1][2] * inverse_inertia_tensor->e[2][0];

    f32 iit_x_transform_11 = transform_matrix->e[1][0] * inverse_inertia_tensor->e[0][1] + 
                              transform_matrix->e[1][1] * inverse_inertia_tensor->e[1][1] + 
                              transform_matrix->e[1][2] * inverse_inertia_tensor->e[2][1];

    f32 iit_x_transform_12 = transform_matrix->e[1][0] * inverse_inertia_tensor->e[0][2] + 
                              transform_matrix->e[1][1] * inverse_inertia_tensor->e[1][2] + 
                              transform_matrix->e[1][2] * inverse_inertia_tensor->e[2][2];

    f32 iit_x_transform_20 = transform_matrix->e[2][0] * inverse_inertia_tensor->e[0][0] + 
                              transform_matrix->e[2][1] * inverse_inertia_tensor->e[1][0] + 
                              transform_matrix->e[2][2] * inverse_inertia_tensor->e[2][0];

    f32 iit_x_transform_21 = transform_matrix->e[2][0] * inverse_inertia_tensor->e[0][1] + 
                              transform_matrix->e[2][1] * inverse_inertia_tensor->e[1][1] + 
                              transform_matrix->e[2][2] * inverse_inertia_tensor->e[2][1];

    f32 iit_x_transform_22 = transform_matrix->e[2][0] * inverse_inertia_tensor->e[0][2] + 
                              transform_matrix->e[2][1] * inverse_inertia_tensor->e[1][2] + 
                              transform_matrix->e[2][2] * inverse_inertia_tensor->e[2][2];
    // At this point the matrix is : iit x transform (right to left multiply order)

    m3x3 *world_inverse_inertia_tensor = &rigid_body->world_inverse_inertia_tensor;
    world_inverse_inertia_tensor->e[0][0] = transform_matrix->e[0][0] * iit_x_transform_00 + 
                                            transform_matrix->e[1][0] * iit_x_transform_10 + 
                                            transform_matrix->e[2][0] * iit_x_transform_20;

    world_inverse_inertia_tensor->e[0][1] = transform_matrix->e[0][0] * iit_x_transform_01 + 
                                            transform_matrix->e[1][0] * iit_x_transform_11 + 
                                            transform_matrix->e[2][0] * iit_x_transform_21;

    world_inverse_inertia_tensor->e[0][2] = transform_matrix->e[0][0] * iit_x_transform_02 + 
                                            transform_matrix->e[1][0] * iit_x_transform_12 + 
                                            transform_matrix->e[2][0] * iit_x_transform_22;

    world_inverse_inertia_tensor->e[1][0] = transform_matrix->e[0][1] * iit_x_transform_00 + 
                                            transform_matrix->e[1][1] * iit_x_transform_10 + 
                                            transform_matrix->e[2][1] * iit_x_transform_20;

    world_inverse_inertia_tensor->e[1][1] = transform_matrix->e[0][1] * iit_x_transform_01 + 
                                            transform_matrix->e[1][1] * iit_x_transform_11 + 
                                            transform_matrix->e[2][1] * iit_x_transform_21;

    world_inverse_inertia_tensor->e[1][2] = transform_matrix->e[0][1] * iit_x_transform_02 + 
                                            transform_matrix->e[1][1] * iit_x_transform_12 + 
                                            transform_matrix->e[2][1] * iit_x_transform_22;

    world_inverse_inertia_tensor->e[2][0] = transform_matrix->e[0][2] * iit_x_transform_00 + 
                                            transform_matrix->e[1][2] * iit_x_transform_10 + 
                                            transform_matrix->e[2][2] * iit_x_transform_20;

    world_inverse_inertia_tensor->e[2][1] = transform_matrix->e[0][2] * iit_x_transform_01 + 
                                            transform_matrix->e[1][2] * iit_x_transform_11 + 
                                            transform_matrix->e[2][2] * iit_x_transform_21;

    world_inverse_inertia_tensor->e[2][2] = transform_matrix->e[0][2] * iit_x_transform_02 + 
                                            transform_matrix->e[1][2] * iit_x_transform_12 + 
                                            transform_matrix->e[2][2] * iit_x_transform_22;
    // At this point the matrix is : inverse transform x iit x transform
}

