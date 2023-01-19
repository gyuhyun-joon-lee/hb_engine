/*
 * Written by Gyuhyun Lee
 */

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

struct CollisionTestResult
{
    f32 hit_t;
    v3 hit_normal;
};

internal CollisionTestResult
collision_test_ray_aabb(AABB *aabb, v3 ray_origin, v3 ray_dir)
{
    CollisionTestResult result = {};

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


