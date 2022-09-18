#include "hb_simulation.h"

// TODO(joon) local space aabb testing?
internal b32
test_aabb_aabb(AABB *a, AABB *b)
{
    b32 result = false;
    v3 half_dim_sum = a->half_dim + b->half_dim; 
    v3 rel_a_center = a->p - b->p;

    // TODO(joon) early return is faster?
    if((rel_a_center.x >= -half_dim_sum.x && rel_a_center.x < half_dim_sum.x) && 
       (rel_a_center.y >= -half_dim_sum.y && rel_a_center.y < half_dim_sum.y) && 
       (rel_a_center.z >= -half_dim_sum.z && rel_a_center.z < half_dim_sum.z))
    {
        result = true;
    }

    return result;
}

internal AABB
init_aabb(v3 center, v3 half_dim, f32 inv_mass)
{
    AABB result = {};

    result.p = center;
    result.half_dim = half_dim;
    result.inv_mass = inv_mass;

    return result;
}

// TODO(joon) temp code, we would want to push later
internal CollisionVolumeCube
init_collision_volume_cube(v3 offset, v3 half_dim)
{
    CollisionVolumeCube result = {};
    result.type = CollisionVolumeType_Cube;
    result.offset = offset;
    result.half_dim = half_dim;

    return result;
}

#if 0
internal void
move_entity(GameState *game_state, Entity *entity, f32 dt_per_frame)
{
    v3 force = compute_force(entity->mass, V3(0, 0, -9.8f));
    entity->v = entity->v + dt_per_frame * (force/entity->mass);

    v3 p_delta = dt_per_frame * entity->v;
    v3 new_p = entity->p + p_delta;
    AABB entity_aabb = entity->aabb;
    entity_aabb.center += entity->p;


    b32 collided = false;
    for(u32 test_entity_index = 0;
            test_entity_index < game_state->entity_count;
            ++test_entity_index)
    {
        Entity *test_entity = game_state->entities + test_entity_index;

        if(test_entity != entity)
        {
            // TODO(joon) collision rule hash!
            if(entity->type == Entity_Type_Voxel && 
                test_entity->type == Entity_Type_Voxel)
            {
                AABB test_entity_aabb = test_entity->aabb;
                test_entity_aabb.center += test_entity->p;
                
                collided = test_aabb_aabb(&entity_aabb, &test_entity_aabb);
            }
            else if(entity->type == Entity_Type_Voxel && 
                    test_entity->type == Entity_Type_Room)
            {
                AABB test_entity_aabb = test_entity->aabb;
                test_entity_aabb.center += test_entity->p;

                collided = !test_aabb_aabb(&entity_aabb, &test_entity_aabb);
            }
        }

        if(collided)
        {
            break;
        }
    }

    if(!collided)
    {
        entity->p = new_p;
    }
}
#endif
// TODO(joon) make this to test against multiple flags?
internal b32
is_entity_flag_set(Entity *entity, EntityFlag flag)
{
    b32 result = false;

    if(entity->flags | flag)
    {
        result = true;
    }

    return result;
}

struct CollTestResult
{
    f32 hit_t;
    v3 hit_normal;
};

internal CollTestResult
collision_test_ray_aabb(AABB *aabb, v3 ray_origin, v3 ray_dir)
{
    CollTestResult result = {};

    v3 aabb_min = aabb->p - aabb->half_dim;
    v3 aabb_max = aabb->p + aabb->half_dim;

    v3 inv_ray_dir = V3(1.0f/ray_dir.x, 1.0f/ray_dir.y, 1.0f/ray_dir.z);
    
    v3 t_0 = hadamard(aabb_max - ray_origin, inv_ray_dir);
    v3 t_1 = hadamard(aabb_min - ray_origin, inv_ray_dir);

    v3 t_min = gather_min_elements(t_0, t_1);
    v3 t_max = gather_max_elements(t_0, t_1);

    f32 t_min_of_max = min_element(t_max);
    f32 t_max_of_min = max_element(t_min);

    if(t_max_of_min < t_min_of_max)
    {
        result.hit_t = t_max_of_min;
        // TODO(joon) proper hit normal calculation
        result.hit_normal = V3(0, 0, 1);
        //result_hit_normal = ;
    }

    return result;
}

struct ClosestPoints
{
    v3 p0; 
    v3 p1; 
};

internal ClosestPoints
get_closest_points_on_two_lines(v3 line0, v3 p0, v3 line1, v3 p1)
{
    ClosestPoints result = {};

    v3 n = cross(line0, line1);
    assert(length_square(n) > 0.0001f); // NOTE(joon) lines cannot be parallel... for now.
    n = normalize(n); // TODO(joon) we dont need to normalize here?

    v3 n0 = cross(n, line0);
    v3 n1 = cross(n, line1);

    result.p0 = p0 + (dot(n1, p1 - p0) / dot(n1, line0)) * line0;
    result.p1 = p1 + (dot(n0, p0 - p1) / dot(n0, line1)) * line1;

    return result;
}

/*
   NOTE(joon) coherency
   We are currently generating only one contact for each collision test,
   but we really want to generate more than one contact point(i.e if an edge meets the face, we will have two vertex face contacts)

   We do this by saving each contact data, 
*/

union CoordinateSystem
{
    struct
    {
        v3 x_axis;
        v3 y_axis;
        v3 z_axis;

        v3 base;
    };

    v3 axes[4];
};

internal void
get_coordinate_system(m4x4 *transform, v3 offset, CoordinateSystem *cs)
{
    // NOTE(joon) make sure that we already filled out the transform matrix!
    cs->x_axis = V3(transform->e[0][0], transform->e[1][0], transform->e[2][0]);
    cs->y_axis = V3(transform->e[0][1], transform->e[1][1], transform->e[2][1]);
    cs->z_axis = V3(transform->e[0][2], transform->e[1][2], transform->e[2][2]);

    cs->base = V3(transform->e[3][0], transform->e[3][1], transform->e[3][2]) + offset; 
}

internal v3
convert_from_coordinate_system(CoordinateSystem *cs, v3 p)
{
    v3 result = p.x * cs->x_axis + p.y * cs->y_axis + p.z * cs->z_axis + cs->base;

    return result;
}

struct ContactData
{
    v3 p;
    v3 normal;
    f32 penetration;
};

// TODO(joon) we will make this to be more general purposed, if we need to. 
internal void
generate_contact_cube_cube(CollisionVolumeCube *cube0, m4x4 *transform0, 
                        CollisionVolumeCube *cube1, m4x4 *transform1)
{
    CoordinateSystem cs0 = {};
    get_coordinate_system(transform0, cube0->offset, &cs0);

    CoordinateSystem cs1 = {};
    get_coordinate_system(transform1, cube1->offset, &cs1);

    CoordinateSystem hd0 = cs0;
    hd0.x_axis *= cube0->half_dim.x;
    hd0.y_axis *= cube0->half_dim.y;
    hd0.z_axis *= cube0->half_dim.z;

    CoordinateSystem hd1 = cs1;
    hd1.x_axis *= cube1->half_dim.x;
    hd1.y_axis *= cube1->half_dim.y;
    hd1.z_axis *= cube1->half_dim.z;

    // NOTE(joon) This _should_ be pointing second from first
    v3 center_to_center = cs1.base - cs0.base;

    // NOTE(joon) cube-cube SAT test needs 15 axes
    v3 seperating_axes[15] = {};
    
    // NOTE(joon) face normals of the first box, already normalized
    seperating_axes[0] = cs0.x_axis;
    seperating_axes[1] = cs0.y_axis;
    seperating_axes[2] = cs0.z_axis;

    // NOTE(joon) face normals of the second box, already normalized
    seperating_axes[3] = cs1.x_axis;
    seperating_axes[4] = cs1.y_axis;
    seperating_axes[5] = cs1.z_axis;

    // NOTE(joon) edge x edge
    // IMPORTANT(joon) the order is really important, as we are going to use the indices
    // for edge - edge case (edge_index_0 = sa_index/3, edge_index_1 = sa_index%3)
    seperating_axes[6] = normalize(cross(cs0.axes[0], cs1.axes[0]));
    seperating_axes[7] = normalize(cross(cs0.axes[0], cs1.axes[1]));
    seperating_axes[8] = normalize(cross(cs0.axes[0], cs1.axes[2]));

    seperating_axes[9] = normalize(cross(cs0.axes[1], cs1.axes[0]));
    seperating_axes[10] = normalize(cross(cs0.axes[1], cs1.axes[1]));
    seperating_axes[11] = normalize(cross(cs0.axes[1], cs1.axes[2]));

    seperating_axes[12] = normalize(cross(cs0.axes[2], cs1.axes[0]));
    seperating_axes[13] = normalize(cross(cs0.axes[2], cs1.axes[1]));
    seperating_axes[14] = normalize(cross(cs0.axes[2], cs1.axes[2]));

    u32 best_sa_index = U32_Max;
    f32 best_penetration = Flt_Max;
    for(u32 sa_index = 0;
            sa_index < array_count(seperating_axes);
            ++sa_index)
    {
        v3 *sa = seperating_axes + sa_index;

        // NOTE(joon) edge x edge axis can be 0, if they were nearly parallel
        if(length_square(*sa) > 0.0001f)
        {
            v3 abs_proj0 = V3(abs(dot(*sa, hd0.x_axis)),
                               abs(dot(*sa, hd0.y_axis)),
                               abs(dot(*sa, hd0.z_axis)));

            v3 abs_proj1 = V3(abs(dot(*sa, hd1.x_axis)),
                               abs(dot(*sa, hd1.y_axis)),
                               abs(dot(*sa, hd1.z_axis)));

            // NOTE(joon) This effectively finds a maximum half dim(x, y, z combined) projection 
            // along the target seperating axis
            f32 max_proj_length0 = abs_proj0.x + abs_proj0.y + abs_proj0.z;
            f32 max_proj_length1 = abs_proj1.x + abs_proj1.y + abs_proj1.z;

            f32 center_to_center_lenth_along_sa = abs(dot(*sa, center_to_center));

            f32 penetration = max_proj_length0 + 
                              max_proj_length1 - 
                              center_to_center_lenth_along_sa;

            if(penetration > 0.0f)
            {
                if(penetration < best_penetration)
                {
                    best_penetration = penetration;
                    best_sa_index = sa_index;
                }
            }
            else
            {
                // NOTE(joon) two cubes are not contacting, exit immediately
                break;
            }
        }
    }

    // NOTE(joon) 
    // 0 ~ 5 : contact normal is face normal
    // 6 ~ 14 : conatct normal is a cross product of two edges
    if(best_sa_index < array_count(seperating_axes)) // NOTE(joon) check if there was any possible contact
    {
        // NOTE(joon) contact normal is _always_ for the first object.
        // for the second object(if there is one), we invert the contact normal 
        v3 contact_normal = seperating_axes[best_sa_index];

        // NOTE(joon) wherever the second cube is, we want the contact normal to be 
        // facing backwards from the contact normal
        if(dot(center_to_center, contact_normal) > 0.0f)
        {
            contact_normal *= -1.0f;
        }

        if(best_sa_index < 3)
        {
            // NOTE(joon) face vertex contact, convex polyhedra cannot produce edge-face or face face contact
            // because of the contact priority order.
            v3 contact_vertex = {};
            for(u32 i = 0;
                    i < 3;
                    ++i)
            {
                f32 sign = 1.0f;
                if(dot(hd1.axes[i], contact_normal) > 0.0f)
                {
                    sign = -1.0f;
                }

                contact_vertex += sign * hd1.axes[i];
            }
            contact_vertex += cs1.base;

            // TODO(joon) fill up the contact buffer
            ContactData contact = {};
            contact.normal = contact_normal;
            contact.p = contact_vertex;
            contact.penetration = best_penetration;
        }
        // TODO(joon) this routine has not been tested 
        else if(best_sa_index >= 3 && best_sa_index < 6)
        {

            // NOTE(joon) face vertex contact, convex polyhedra cannot produce edge-face or face face contact
            // because of the contact priority order.

            // NOTE(joon) opposite from the first case, because now the first cube is providing the vertex
            v3 contact_vertex = {};
            for(u32 i = 0;
                    i < 3;
                    ++i)
            {
                f32 sign = 1.0f;
                if(dot(hd0.axes[i], contact_normal) > 0.0f)
                {
                    sign = -1.0f;
                }

                contact_vertex += sign * hd0.axes[i];
            }
            contact_vertex += cs0.base;

            // TODO(joon) fill up the contact buffer
            ContactData contact = {};
            contact.normal = contact_normal;
            contact.p = contact_vertex;
            contact.penetration = best_penetration;
        }
        else
        {
            // NOTE(joon) edge x edge contact
            u32 edge_index0 = (best_sa_index - 6) / 3;
            u32 edge_index1 = best_sa_index % 3;

            v3 p_on_edge0 = {};
            v3 p_on_edge1 = {};
            for(u32 axis_index = 0;
                    axis_index < 3;
                    ++axis_index)
            {
                if(axis_index == edge_index0)
                {
                    p_on_edge0.e[axis_index] = 0.0f;
                }
                else if(dot(hd0.axes[axis_index], contact_normal) > 0.0f)
                {
                    p_on_edge0 -= hd0.axes[axis_index];
                }
                else
                {
                    p_on_edge0 += hd0.axes[axis_index];
                }

                if(axis_index == edge_index1)
                {
                    p_on_edge1.e[axis_index] = 0.0f;
                }
                else if(dot(hd1.axes[axis_index], contact_normal) < 0.0f)
                {
                    p_on_edge1 -= hd1.axes[axis_index];
                }
                else
                {
                    p_on_edge1 += hd1.axes[axis_index];
                }
            }
            p_on_edge0 += cs0.base;
            p_on_edge1 += cs1.base;

            // NOTE(joon) get the closest points on two lines
            ClosestPoints closest_points = get_closest_points_on_two_lines(cs0.axes[edge_index0], p_on_edge0, cs1.axes[edge_index1], p_on_edge1);

            ContactData contact = {};
            contact.p = 0.5f*(closest_points.p0 + closest_points.p1);
            contact.normal = contact_normal;
            contact.penetration = best_penetration;
        }
    }
}

/*
    NOTE(joon) inertia tensor
    Moment of inertia, which replaces the mass in F = ma in the subsequent equation t(torque) = I(moment of inertia) * w(angular v),
    can be represented in a m3x3 matrix called intertia tensor
    |Ix  -Ixy -Ixz|
    |-Ixy Iy  -Iyz|
    |-Ixz  -Iyz  Iz|

    where Iab = sum(mi * pi.a * pi.b) (for principle axes x, y, and z, we just put the same axis in both a and b)
*/
// TODO(joon) also add initial orietation
// TODO(joon) instead of returning a rigid body, allocate the space and return the pointer
internal RigidBody
init_rigid_body(v3 p, f32 inv_mass, m3x3 inertia_tensor)
{
    RigidBody result = {};
    result.p = p;
    result.inv_mass = inv_mass;
    result.orientation = Quat(0, V3(1, 0, 0));

    if(!compare_with_epsilon(inv_mass, 0.0f))
    {
        result.inv_inertia_tensor = inverse(inertia_tensor);
    }
    else
    {
        // TODO(joon) double check if this is right, though it looks correct?
        result.inv_inertia_tensor = {};
    }

    return result;
}

internal void
init_rigid_body_and_calculate_derived_parameters_for_frame(RigidBody *rb)
{
    // TODO(joon) is applying the drag to dp 'here' correct?
    f32 linear_damp = 0.99f;
    rb->dp *= linear_damp;

    f32 angular_damp = 0.99f;
    rb->angular_dp *= angular_damp;

    rb->orientation = normalize(rb->orientation);

    // NOTE(joon) transform matrix
    m3x3 rot = rotation_quat_to_m3x3(rb->orientation);
    rb->transform = M4x4(rot);
    rb->transform.e[0][3] = rb->p.x;
    rb->transform.e[1][3] = rb->p.y;
    rb->transform.e[2][3] = rb->p.z;
    rb->transform.e[3][3] = 1.0f;

    // transform inv inertia tensor
    m3x3 transform = M3x3(rb->transform); // NOTE(joon) we dont care about the translation here
    rb->transform_inv_inertia_tensor = transform* // transform back to world space
                                        rb->inv_inertia_tensor* // both inertia & vector that we are multiplying are in local space
                                        inverse(transform); // transform to local space
}

internal void
add_force_at_local_p(RigidBody *rb, v3 force, v3 local_p)
{
    rb->force += force;

    v3 world_p = (rb->transform * V4(local_p, 1.0f)).xyz;
    v3 rel_world_p = world_p - rb->p;

    rb->torque += cross(rel_world_p, force);
}












