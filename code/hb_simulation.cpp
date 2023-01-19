/*
 * Written by Gyuhyun Lee
 */

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











