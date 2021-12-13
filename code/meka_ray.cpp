struct material
{
    r32 reflectivity; // 0.0f being very rough like a chalk, and 1 being really relfective(like mirror)
    v3 emit_color; // things like sky, lightbulb have this value
    v3 reflection_color; // most of the other things in real life, that does not have enough energy to emit themselves
};

// TODO(joon): SIMD these!
struct plane
{
    v3 normal;
    r32 d;

    u32 material_index;
};

struct sphere
{
    v3 center;
    r32 r;

    u32 material_index;
};

struct triangle
{
    v3 v0;
    v3 v1;
    v3 v2;

    u32 material_index;
};

struct world
{
    material *materials;
    u32 material_count;

    u32 plane_count;
    plane *planes;

    u32 sphere_count;
    sphere *spheres;

    u32 triangle_count;
    triangle *triangles;

    u32 total_tile_count;
    volatile u32 rendered_tile_count;

    volatile u64 total_ray_count;
    volatile u64 bounced_ray_count;
};

struct ray_intersect_result
{
    // NOTE(joon): instead of having a boolean value inside the struct, this value will be initialized to a negative value
    // and when we want to check if there was a hit, we just check if hit_t >= 0.0f 
    r32 hit_t;

    v3 next_normal;
};

// TODO(joon) : This works for both inward & outward normals, 
// but we might only want to test it against the outward normal
internal ray_intersect_result
ray_intersect_with_triangle(v3 v0, v3 v1, v3 v2, v3 ray_origin, v3 ray_dir)
{
    /*
       NOTE(joon) : 
       Moller-Trumbore line triangle intersection argorithm

       |t| =       1           |(T x E1) * E2|
       |u| = -------------  x  |(ray_dir x E2) * T |
       |v| = (ray_dir x E2)    |(T x E1) * ray_dir |

       where T = ray_origin - v0, E1 = v1 - v0, E2 = v2 - v0,
       ray  = ray_origin + t * ray_dir;

       u & v = barycentric coordinates of the triangle, as a triangle can be represented in a form of 
       (1 - u - v)*v0 + u*v1 + v*v2;

       Note that there are a lot of same cross products, which we can calculate just once and reuse
       v0, v1, v2 can be in any order
    */
    r32 hit_t_threshold = 0.0001f;

    ray_intersect_result result = {};
    result.hit_t = -1.0f;

    v3 e1 = v1 - v0;
    v3 e2 = v2 - v0;
    v3 cross_ray_e2 = cross(ray_dir, e2);

    r32 det = dot(cross_ray_e2, e1);
    v3 T = ray_origin - v0;

    // TODO(joon) : completely made up number
    r32 tolerance = 0.0001f;
    if(det <= -tolerance || det >= tolerance) // if the determinant is 0, it means the ray is parallel to the triangle
    {
        v3 a = cross(T, e1);

        r32 t = dot(a, e2) / det;
        r32 u = dot(cross_ray_e2, T) / det;
        r32 v = dot(a, ray_dir) / det;

        if(t >= hit_t_threshold && u >= 0.0f && v >= 0.0f && u+v <= 1.0f)
        {
            result.hit_t = t;
            
            // TODO(joon): calculate normal based on the ray dir, so that the next normal is facing the incoming ray dir
            // otherwise, the reflection vector will be totally busted?
            result.next_normal = normalize(cross(e1, e2));
        }
    }

    return result;
}


internal ray_intersect_result
ray_intersect_with_plane(v3 normal, r32 d, v3 ray_origin, v3 ray_dir)
{
    ray_intersect_result result = {};
    result.hit_t = -1.0f;
    r32 hit_t_threshold = 0.0001f;

    // NOTE(joon) : if the denominator is 0, it means that the ray is parallel to the plane
    r32 denom = dot(normal, ray_dir);

    r32 tolerance = 0.00001f;
    if(denom < -tolerance || denom > tolerance)
    {
        r32 t = (dot(-normal, ray_origin) - d)/denom;
        if(t >= hit_t_threshold)
        {
            result.hit_t = t;
        }
    }

    return result;
}

// NOTE(joon) : c(center of the sphere), r(radius of the sphere)
internal ray_intersect_result
ray_intersect_with_sphere(v3 center, r32 r, v3 ray_origin, v3 ray_dir)
{
    r32 hit_t_threshold = 0.0001f;

    ray_intersect_result result = {};
    result.hit_t = -1.0f;
    
    v3 rel_ray_origin = ray_origin - center;
    r32 a = dot(ray_dir, ray_dir);
    r32 b = 2.0f*dot(ray_dir, rel_ray_origin);
    r32 c = dot(rel_ray_origin, rel_ray_origin) - r*r;

    r32 root_term = b*b - 4.0f*a*c;

    r32 tolerance = 0.00001f;
    if(root_term > tolerance)
    {
        // two intersection points
        r32 t = (-b - sqrt(root_term))/(2*a);
        if(t > hit_t_threshold)
        {
            result.hit_t = t;
        }
    }
    else if(root_term < tolerance && root_term > -tolerance)
    {
        // one intersection point
        r32 t = (-b)/(2*a);
        if(t > hit_t_threshold)
        {
            result.hit_t = t;
        }
    }
    else
    {
        // no intersection
    }

    return result;
}

// TODO(joon): is this really all for linear to srgb?
internal r32
linear_to_srgb(r32 linear_value)
{
    r32 result = 0.0f;

    assert(linear_value >= 0.0f && linear_value <= 1.0f);

    if(linear_value >= 0.0f && linear_value <= 0.0031308f)
    {
        result = linear_value * 12.92f;
    }
    else
    {
        result = 1.055f*pow(linear_value, 1/2.4f) - 0.055f;
    }

    return result;
}

internal void
raytrace_per_pixel()
{
}

struct raytracer_data
{
    world *world;
    u32 *pixels; 
    u32 output_width; 
    u32 output_height;
    u32 ray_per_pixel_count;

    v3 film_center; 
    r32 film_width; 
    r32 film_height;

    v3 camera_p;
    v3 camera_x_axis; 
    v3 camera_y_axis; 
    v3 camera_z_axis;

    u32 min_x; 
    u32 one_past_max_x;
    u32 min_y; 
    u32 one_past_max_y;

};

struct raytracer_output
{
    u64 bounced_ray_count;
};

internal raytracer_output
render_raytraced_image_tile(raytracer_data *data)
{
    raytracer_output result = {};

    world *world = data->world;
    u32 *pixels = data->pixels; 
    u32 output_width = data->output_width; 
    u32 output_height = data->output_height;
    u32 ray_per_pixel_count = data->ray_per_pixel_count;

    v3 film_center = data->film_center; 
    r32 film_width = data->film_width; 
    r32 film_height = data->film_height;

    v3 camera_p = data->camera_p;
    v3 camera_x_axis = data->camera_x_axis; 
    v3 camera_y_axis = data->camera_y_axis; 
    v3 camera_z_axis = data->camera_z_axis;

    u32 min_x = data->min_x;  
    u32 one_past_max_x = data->one_past_max_x;
    u32 min_y = data->min_y; 
    u32 one_past_max_y = data->one_past_max_y;

    // TODO(joon): Get rid of rand()
    u32 random_seed = (u32)rand();
    random_series series = start_random_series(random_seed); 

    // TODO(joon): These values should be more realistic
    // for example, when we ever have a concept of an acutal senser size, 
    // we should use that value to calculate this!
    r32 x_per_pixel = film_width/output_width;
    r32 y_per_pixel = film_height/output_height;

    r32 half_film_width = film_width/2.0f;
    r32 half_film_height = film_height/2.0f;

    u32 *row = pixels + min_y*output_width + min_x;

    // NOTE(joon): This is an _extremely_ hot loop. Very small performance increase/decrease can have a huge impact on the total time
    u32 bounced_ray_count = 0;
    for(u32 y = min_y;
            y < one_past_max_y;
            ++y)
    {
        r32 film_y = 2.0f*((r32)y/(r32)output_height) - 1.0f;
        u32 *pixel = row;

        if(y == 1070)
        {
            int a = 1;
        }

        for(u32 x = min_x;
                x < one_past_max_x;
                ++x)
        {
            v3 result_color = V3(0, 0, 0);

            for(u32 ray_per_pixel_index = 0;
                    ray_per_pixel_index < ray_per_pixel_count;
                    ++ray_per_pixel_index)
            {
                // NOTE(joon): These values are inside the loop because as we are casting multiple lights anyway,
                // we can slightly 'jitter' the ray direction to get the anti-aliasing effect 
                r32 film_x = 2.0f*((r32)x/(r32)output_width) - 1.0f;

                r32 jitter_x = x_per_pixel*random_between_0_1(&series);
                r32 jitter_y = x_per_pixel*random_between_0_1(&series);
                // NOTE(joon): we multiply x and y value by half film dim(to get the position in the sensor which is center oriented) & 
                // axis(which are defined in world coordinate, so by multiplying them, we can get the world coodinates)
                v3 film_p = film_center + (film_y + jitter_y)*half_film_height*camera_y_axis + (film_x + jitter_x)*half_film_width*camera_x_axis;

                // TODO(joon) : later on, we need to make this to be random per ray per pixel!
                v3 ray_origin = camera_p;
                v3 ray_dir = normalize(film_p - camera_p);
                v3 attenuation = V3(1, 1, 1);

                for(u32 ray_bounce_index = 0;
                        ray_bounce_index < 8;
                        ++ray_bounce_index)
                {
                    r32 min_hit_t = flt_max;

                    u32 hit_mat_index = 0;

                    // NOTE(joon): Want to get rid of this value.. but sphere needs this to calculate the normal :(
                    v3 next_ray_origin = {};
                    v3 next_normal = {};

                    for(u32 plane_index = 0;
                            plane_index < world->plane_count;
                            ++plane_index)
                    {
                        plane *plane = world->planes + plane_index;

                        ray_intersect_result intersect_result = ray_intersect_with_plane(plane->normal, plane->d, ray_origin, ray_dir);
                        
                        if(intersect_result.hit_t >= 0.0f && intersect_result.hit_t < min_hit_t)
                        {
                            hit_mat_index = plane->material_index;
                            min_hit_t = intersect_result.hit_t;

                            next_ray_origin = ray_origin + intersect_result.hit_t*ray_dir;
                            next_normal = plane->normal;
                        }
                    }

                    for(u32 sphere_index = 0;
                            sphere_index < world->sphere_count;
                            ++sphere_index)
                    {
                        sphere *sphere = world->spheres + sphere_index;

                        ray_intersect_result intersect_result = ray_intersect_with_sphere(sphere->center, sphere->r, ray_origin, ray_dir);
                        
                        if(intersect_result.hit_t >= 0.0f && intersect_result.hit_t < min_hit_t)
                        {
                            hit_mat_index = sphere->material_index;
                            min_hit_t = intersect_result.hit_t;

                            next_ray_origin = ray_origin + intersect_result.hit_t*ray_dir;
                            next_normal = next_ray_origin - sphere->center;
                        }
                    }

                    for(u32 triangle_index = 0;
                            triangle_index < world->triangle_count;
                            ++triangle_index)
                    {
                        triangle *triangle = world->triangles + triangle_index;

                        ray_intersect_result intersect_result = ray_intersect_with_triangle(triangle->v0, triangle->v1, triangle->v2, ray_origin, ray_dir);
                        
                        if(intersect_result.hit_t >= 0.0f && intersect_result.hit_t < min_hit_t)
                        {
                            hit_mat_index = triangle->material_index;
                            min_hit_t = intersect_result.hit_t;

                            next_ray_origin = ray_origin + intersect_result.hit_t*ray_dir;
                            next_normal = intersect_result.next_normal;
                        }
                    }


                    if(hit_mat_index)
                    {
                        material *hit_material = world->materials + hit_mat_index;
                        // TODO(joon): cos?
                        result_color += hadamard(attenuation, hit_material->emit_color); 
                        attenuation = hadamard(attenuation, hit_material->reflection_color);

                        ray_origin = next_ray_origin;

                        next_normal = normalize(next_normal);
                        v3 perfect_reflection = normalize(ray_dir -  2.0f*dot(ray_dir, next_normal)*next_normal);
                        v3 random_reflection = normalize(next_normal + V3(random_between_minus_1_1(&series), random_between_minus_1_1(&series), random_between_minus_1_1(&series)));

                        ray_dir = lerp(random_reflection, hit_material->reflectivity, perfect_reflection);

                        bounced_ray_count++;
                        //ray_dir = perfect_reflection;
                    }
                    else
                    {
                        if(ray_bounce_index == 0)
                        {
                            int a = 1;
                        }

                        result_color += hadamard(attenuation, world->materials[0].emit_color); 
                        break;
                    }
                }
            }

            result_color /= (r32)ray_per_pixel_count;

            result_color.r = clamp01(result_color.r);
            result_color.g = clamp01(result_color.g);
            result_color.b = clamp01(result_color.b);
#if 1
            result_color.r = linear_to_srgb(result_color.r);
            result_color.g = linear_to_srgb(result_color.g);
            result_color.b = linear_to_srgb(result_color.b);
#endif

            u32 result_r = round_r32_u32(255.0f*result_color.r) << 16;
            u32 result_g = round_r32_u32(255.0f*result_color.g) << 8;
            u32 result_b =  round_r32_u32(255.0f*result_color.b) << 0;

            u32 result_pixel_color = 0xff << 24 |
                                    result_r |
                                    result_g |
                                    result_b;

            *pixel++ = result_pixel_color;
        }

        row += output_width;
    }

    result.bounced_ray_count = bounced_ray_count;

    return result;
}













