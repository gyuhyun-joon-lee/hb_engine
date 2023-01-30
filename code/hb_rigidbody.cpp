#include "hb_rigidbody.h"

/*
   NOTE(gh) These are the local_space inverse inertia tensor of some of the most common shapes!
   
*/
inline m3x3
get_inverse_inertia_tensor_dense_sphere(f32 radius, f32 mass)
{
    f32 c = (2.0f/5.0f)*square(radius)*mass;

    m3x3 result = M3x3(c, 0, 0,
                       0, c, 0,
                       0, 0, c);

    return result;
}

inline m3x3
get_inverse_inertia_tensor_shell_sphere(f32 radius, f32 mass)
{
    f32 c = (2.0f/3.0f)*square(radius)*mass;

    m3x3 result = M3x3(c, 0, 0,
                       0, c, 0,
                       0, 0, c);

    return result;
}

inline m3x3
get_inverse_inertia_tensor_ellipsoid(f32 radius_x, f32 radius_y, f32 radius_z, f32 mass)
{
    f32 a = (1.0f/5.0f)*(square(radius_y) + square(radius_z))*mass;
    f32 b = (1.0f/5.0f)*(square(radius_x) + square(radius_z))*mass;
    f32 c = (1.0f/5.0f)*(square(radius_x) + square(radius_y))*mass;

    m3x3 result = M3x3(a, 0, 0,
                       0, b, 0,
                       0, 0, c);

    return result;
}

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

internal v3
get_world_space_center_of_mass(RigidBody *rigid_body)
{
    v3 result = rigid_body->entity->position + rigid_body->center_of_mass_offset;

    return result;
}

internal void
apply_force_at_world_space_point(RigidBody *rigid_body,
                                v3 force, // linear + angular, world space
                                v3 world_p)
{
    v3 center_of_mass = get_world_space_center_of_mass(rigid_body);
    rigid_body->force += force;
    // TODO(gh) If the COM was very close to the point where the force is being applied,
    // this might cause unwanted torque
    rigid_body->torque += cross(world_p - center_of_mass, force);
}

internal void
apply_force_at_local_space_point(RigidBody *rigid_body,
                                v3 force, // linear + angular, world space
                                v3 local_p)
{
}

// NOTE(gh) Doesn't cause the object to rotate. Common thing that uses this path would be gravity
internal void
apply_force_at_center_of_mass(RigidBody *rigid_body,
                              v3 force // world space
                              )
{
    rigid_body->force += force;
}

internal void
initialize_rigid_body(Entity *entity, RigidBody *rigid_body,
                      f32 inverse_mass,
                      m3x3 *inverse_inertia_tensor)
{
    rigid_body->entity = entity;
    rigid_body->inverse_mass = inverse_mass;
    rigid_body->center_of_mass_offset = V3(0, 0, 0);
    rigid_body->linear_velocity = V3(0, 0, 0);
    rigid_body->linear_damping = 0.99f;

    rigid_body->orientation = Quat(1, 0, 0, 0);
    // TODO(gh) Properly set inverse inertia tensor!!
    rigid_body->inverse_inertia_tensor = *inverse_inertia_tensor;
    // TODO(gh) Set up angular damping as well
    rigid_body->angular_damping = 0.99f;

    rigid_body->force = V3(0, 0, 0);
    rigid_body->torque = V3(0, 0, 0);
}

internal void
update_derived_rigid_body_attributes(RigidBody *rigid_body)
{
    // NOTE(gh) Quaternion has 4 DOF, but we want 3 DOF, so we normalize the quaternion (4-1=3)
    rigid_body->orientation = normalize(rigid_body->orientation);

    /*
       NOTE(gh) Update transform matrix
    */
    v3 center_of_mass = get_world_space_center_of_mass(rigid_body);
    m3x4 *transform_matrix = &rigid_body->transform_matrix;
    quat *orientation = &rigid_body->orientation;
    transform_matrix->e[0][0] = 1 - 2.0f*(orientation->y*orientation->y + orientation->z*orientation->z);
    transform_matrix->e[0][1] = 2.0f*(orientation->x*orientation->y - orientation->s*orientation->z);
    transform_matrix->e[0][2] = 2.0f*(orientation->x*orientation->z + orientation->s*orientation->y);
    transform_matrix->e[0][3] = center_of_mass.x;

    transform_matrix->e[1][0] = 2.0f*(orientation->x*orientation->y + orientation->s*orientation->z);
    transform_matrix->e[1][1] = 1 - 2.0f*(orientation->x*orientation->x + orientation->z*orientation->z);
    transform_matrix->e[1][2] = 2.0f*(orientation->y*orientation->z - orientation->s*orientation->x);
    transform_matrix->e[1][3] = center_of_mass.y;

    transform_matrix->e[2][0] = 2.0f*(orientation->x*orientation->z - orientation->s*orientation->y);
    transform_matrix->e[2][1] = 2.0f*(orientation->y*orientation->z + orientation->x*orientation->s);
    transform_matrix->e[2][2] = 1 - 2.0f*(orientation->x*orientation->x + orientation->y*orientation->y);
    transform_matrix->e[2][3] = center_of_mass.z;

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

// TODO(gh) Double check if the math here checks out
internal void
integrate_rigid_body(RigidBody *rigid_body, f32 dt)
{
    // linear acc = force / mass
    v3 linear_acceleration = rigid_body->inverse_mass * rigid_body->force;

    // angular acc = inverse inertia tensor * torque
    v3 angular_acceleration = rigid_body->world_inverse_inertia_tensor * rigid_body->torque;

    // new linear velocity = previous linear velocity + linear acc * dt
    rigid_body->linear_velocity += dt*linear_acceleration;

    // new angular velocity = preivous angular velocity + angular acc * dt
    rigid_body->angular_velocity += dt*angular_acceleration;

    // apply damping
    rigid_body->linear_velocity *= power(rigid_body->linear_damping, dt);
    rigid_body->angular_velocity *= power(rigid_body->angular_damping, dt);

    // Get new position & orientation out of updated linear & angular velocities
    // new position = previous position + linear velocity * dt
    // TODO(gh) For now, we are offsetting the 'offset' of the COM relative to the entity position.
    // What if we want to move the entity position instead of just this rigid body? 
    // i.e enemy gets knocked away when hit by the player
    rigid_body->center_of_mass_offset += dt*rigid_body->linear_velocity;

    // new orientation = previous orientation + (dt/2)*angular velocity(represented in pure quaternion)*preivous orientation
    // NOTE(gh) Note that this integration is not as simple as linear velocity(like av = av + av*t or something),
    // as this is a quarternion integration.
    // For more explanation, see https://fgiesen.wordpress.com/2012/08/24/quaternion-differentiation/
    rigid_body->orientation += 0.5f*dt*Quat(0, rigid_body->angular_velocity)*rigid_body->orientation;
}

/*
    NOTE(gh) How collision impacts the objects that were involved in impulse-based physics engine
    When the two objects collide, the deformation of two objects results in a large amount of force 
    for a very short period of time. As it is hard to simulate this in our game engine with fixed time step,
    we instead use a thing called impulse, which is a change in momentum. You can also get this t

    Terms : 
    - J = impulse
    - n = contact normal(unit vector)
    - p = contact point
    - - = Any property before the collision
    - + = Any property after the collision
    - c = Collision restitution, 1 being complete bounce and 0 being no bounce
    - iit = inverse inertia tensor
    - com = center of mass
    - m = mass
    - r = (p - com)

    Equations that we know : 
    - Impulse J = j * n, Impulse works only along the contact normal)
    - Impulsive torque = r x J, Very similar to the torque being r x F
    - v_a and v_b = linear velocity + angular velocity x r, so the velocity on any point is consisted of linear+angular velocity
    - new linear velocity = old + J/m
    - new angular velocity = old + iit*(r x J)

    - v_rel(-) = n * (v_a(-)-v_b(-))
    - v_rel(+) = n * (v_a(+)-v_b(+))
    - v_rel(+) = -c*v_rel(-), v_rel is a scalar value

    So by representing v_rel(+), v_a(+), v_b(+) in v_rel(-), v_a(-), v_b(-)
    we can use the equation v_rel(+) = -c*v_rel(-) to get j. To be more specific,
    - v_a(+) = old linear velocity + J/m + (old angular velocity + iit*(r x J) x r = 
      old linear velocity + old angular velcocity x r + J/m + iit*(r x J) x r = 
      v_a(-) + J/m_a + iit_a*(r_a x J) x r_a
    You can do the same thing for v_b(+), plug these in to v_rel(+) to represent it in v_rel(-)!

    After the whole calculations, we get 
    j = -(1+c)*v_rel(-) / (A+B+C),
    where 
    A = 1/m_a + 1/m_b
    B = n * ((iit_a*(r_a x n)) x r_a)
    C = n * ((iit_b*(r_b x n)) x r_b)

    Now we have j, we can get J by 
    J = j * n

    Note that impulse is a change in momentum, which means an instant change in velocity considering that the mass is the same.
*/

/*
    NOTE(gh) How we are going to update velocity & position (both linear and angular) 
    Original algorithm : https://graphics.stanford.edu/papers/rigid_bodies-sig03/

    To summarize, we are going to update velocity & position with the collisions & contacts in these steps

    1. Collision detection in non-chronological order.
       The temporary position will be x + dt*(v + dt*a), which means we will be using the updated velocity(v + dt*a)
       instead of the current velocity(v) to check for interference.

       If there is collision, we will apply the impulse to the current velocity(v) 
       instead of the updated velocity that we used. 
       If not, we will advance the velocity, which means the current velocity = updated velocity(v + dt*a)
       
    2. Contact resolution using the velocity that the collision dectection has produced.
       Note that this means the collision detection & contact resolution shoud end up using the same velocity
       _if there was no collision_.

    3. Advance the positions.

    The collisions are _not_ resolved in chronological order.
    For example, even if A and B collided first then B and C collided in one time step, 
    the order that we process the collisions might be different.
*/
internal void
move_rigid_body(GameState *game_state, MemoryArena *arena, 
                RigidBody *rigid_body, f32 dt)
{
    update_derived_rigid_body_attributes(rigid_body);

    integrate_rigid_body(rigid_body, dt);

    ContactGroup contact_group = {};
    start_contact_group(&contact_group, arena, kilobytes(64));

    // TODO(gh) Problem with handling the collision later is that we can't get valid information
    // to figure out whether the two objects in resting contact.
    Entity *entity = rigid_body->entity;
    for(u32 test_entity_index = 0;
            test_entity_index < game_state->entity_count;
            ++test_entity_index)
    {
        Entity *test_entity = game_state->entities + test_entity_index;

        // NOTE(gh) For now, we are only allowing the rigid bodies to collide each other
        if(entity != test_entity && 
           test_entity->rigid_body)
        {
            generate_contact_if_applicable(&contact_group, rigid_body, test_entity->rigid_body);
        }
    }

    // Clear accumulated attributes
    rigid_body->force = V3(0, 0, 0);
    rigid_body->torque = V3(0, 0, 0);
}

















