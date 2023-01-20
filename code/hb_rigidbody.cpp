#include "hb_rigidbody.h"

/*
   TODO(gh)
   I guess this also means that we need to convert orientation quaternion -> matrix function later.
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

