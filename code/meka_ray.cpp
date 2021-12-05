internal b32
ray_intersect_with_triangle(v3 v0, v3 v1, v3 v2, v3 ray_start, v3 ray_dir)
{
    /*
       NOTE(joon) : 
       Moller-Trumbore line triangle intersection argorithm

       |t| =    1        |(T x E1) * E2|
       |u| = -------  x  |(R x E2) * T |
       |v| = (R x E2)    |(T x E1) * R |

       where T = ray_start - v0, E1 = v1 - v0, E2 = v2 - v0 and v0, v1, v2 are in _counter clockwise order_
       ray  = ray_start + t * R;

       u & v = barycentric coordinates of the triangle, as a triangle can be represented in a form of 
       (1 - u - v)*v0 + u*v1 + v*v2;

       Note that there are a lot of same cross products, which we can store and save some calculations
    */
    b32 result = false;

    v3 e1 = v1 - v0;
    v3 e2 = v2 - v0;
    v3 cross_ray_e2 = cross(ray_dir, e2);

    r32 det = dot(cross_ray_e2, e1);
    v3 T = ray_start - v0;

    // TODO(joon) : completely made up number
    r32 tolerance = 0.0001f;
    if(det <= tolerance || det > tolerance) // if the determinant is 0, it means the ray is parallel to the triangle
    {
        v3 a = cross(T, e1);

        r32 t = dot(a, e2) / det;
        r32 u = dot(cross_ray_e2, T) / det;
        r32 v = dot(a, ray_dir) / det;

        if(t >= 0.0f && u >= 0.0f && v >= 0.0f && u+v <= 1.0f)
        {
            result = true;
        }
    }

    return result;
}
