
#if 0
#include <limits.h>
#include <stdint.h>
#include <float.h>
#include "meka_types.h"
#include "meka_simd.h"
#include "meka_platform.h"
#include "meka_math.h"
#include "meka_random.h"
#endif

struct material
{
    r32 reflectivity; // 0.0f being very rough like a chalk, and 1 being really relfective(like mirror)
    v3 emit_color; // things like sky, lightbulb have this value
    v3 reflection_color;

    r32 *brdf_table;
};

// TODO(joon): SIMD these!
struct plane
{
    r32 d;
    v3 normal;

    u32 material_index;
};

struct sphere
{
    v3 center;
    r32 radius;

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

    plane *planes;
    u32 plane_count;

    sphere *spheres;
    u32 sphere_count;

    triangle *triangles;
    u32 triangle_count;

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
    r32 sqrt_root_term = sqrt(root_term);


    r32 tolerance = 0.00001f;
    if(root_term > tolerance)
    {
        r32 tn = (-b - sqrt_root_term)/a;
        r32 tp = (-b + sqrt_root_term)/a;

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

    simd_random_series series;
};

struct raytracer_output
{
    u64 bounced_ray_count;
};

internal raytracer_output
render_raytraced_image_tile_simd(raytracer_data *data)
{
    raytracer_output result = {};

    // NOTE(joon): constants
    simd_f32 simd_f32_2 = simd_f32_(2.0f);
    simd_f32 simd_f32_0 = simd_f32_(0.0f);
    simd_f32 simd_f32_1 = simd_f32_(1.0f);
    simd_u32 simd_u32_0 = simd_u32_(0);
    simd_u32 simd_u32_max = simd_u32_(u32_max);

    simd_v3 simd_v3_0 = simd_v3_(v3_(0.0f, 0.0f, 0.0f));

    // TODO(joon) : completely made up number
    simd_f32 square_root_tolerance = simd_f32_(0.00001f);

    world *world = data->world;
    u32 *pixels = data->pixels; 
    u32 output_width = data->output_width; 
    u32 output_height = data->output_height;
    u32 ray_per_pixel_count = data->ray_per_pixel_count;
    assert(ray_per_pixel_count % MEKA_LANE_WIDTH == 0);

    simd_u32 simd_output_width = simd_u32_(output_width);
    simd_u32 simd_output_height = simd_u32_(output_height);
    simd_u32 simd_u32_ray_per_pixel_count = simd_u32_(ray_per_pixel_count);
    simd_f32 simd_f32_ray_per_pixel_count = simd_f32_((f32)ray_per_pixel_count);

    simd_v3 film_center = simd_v3_(data->film_center); 
    simd_f32 film_width = simd_f32_(data->film_width); 
    simd_f32 film_height = simd_f32_(data->film_height);

    simd_v3 camera_p = simd_v3_(data->camera_p);
    simd_v3 camera_x_axis = simd_v3_(data->camera_x_axis); 
    simd_v3 camera_y_axis = simd_v3_(data->camera_y_axis); 
    simd_v3 camera_z_axis = simd_v3_(data->camera_z_axis);

    u32 min_x = data->min_x; 
    u32 one_past_max_x = data->one_past_max_x;
    u32 min_y = data->min_y; 
    u32 one_past_max_y = data->one_past_max_y;


    // TODO(joon): These values should be more realistic. For example, when we ever have a concept of an acutal senser size, 
    // we should use that value to calculate this!
    simd_f32 x_per_pixel = film_width/convert_f32_from_u32(simd_output_width);
    simd_f32 y_per_pixel = film_height/convert_f32_from_u32(simd_output_height);

    simd_f32 half_film_width = film_width/simd_f32_2;
    simd_f32 half_film_height = film_height/simd_f32_2;

    // TODO(joon) : completely made up number
    simd_f32 hit_t_threshold = simd_f32_(0.0001f);

    simd_random_series *series = &data->series;

    u32 *row = pixels + min_y*output_width + min_x;

    // NOTE(joon): This is an _extremely_ hot loop. Very small performance increase/decrease can have a huge impact on the total time
    u32 bounced_ray_count = 0;
    for(u32 y = min_y;
            y < one_past_max_y;
            ++y)
    {
        simd_f32 film_y = simd_f32_(2.0f*((r32)y/(r32)output_height) - 1.0f);
        u32 *pixel = row;

        for(u32 x = min_x;
                x < one_past_max_x;
                ++x)
        {
            simd_f32 film_x = simd_f32_(2.0f*((r32)x/(r32)output_width) - 1.0f);
            simd_v3 result_color = simd_v3_0;

            for(u32 ray_per_pixel_index = 0;
                    ray_per_pixel_index < ray_per_pixel_count;
                    ray_per_pixel_index += MEKA_LANE_WIDTH)
            {
                // NOTE(joon): These values are inside the loop because as we are casting multiple lights anyway,
                // we can slightly 'jitter' the ray direction to get the anti-aliasing effect 
                simd_f32 jitter_x = x_per_pixel*random_between_0_1(series);
                simd_f32 jitter_y = y_per_pixel*random_between_0_1(series);

                // NOTE(joon): we multiply x and y value by half film dim(to get the position in the sensor which is center oriented) & 
                // axis(which are defined in world coordinate, so by multiplying them, we can get the world coodinates)
                simd_v3 film_p = film_center + (film_y + jitter_y)*half_film_height*camera_y_axis + (film_x + jitter_x)*half_film_width*camera_x_axis;

                // TODO(joon) : later on, we need to make this to be random per ray per pixel!
                simd_v3 ray_origin = camera_p;
                simd_v3 ray_dir = film_p - camera_p;
                simd_v3 attenuation = simd_v3_(v3_(1.0f, 1.0f, 1.0f));

                simd_u32 is_ray_alive_mask = simd_u32_max;

                for(u32 bounce_index = 0;
                        bounce_index < 8;
                        ++bounce_index)
                {
                    simd_f32 min_hit_t = simd_f32_(flt_max);

                    simd_u32 hit_mat_index = simd_u32_0;

                    // NOTE(joon): Want to get rid of this value.. but sphere needs this to calculate the normal :(
                    simd_v3 next_ray_origin = simd_v3_0;
                    simd_v3 next_normal = simd_v3_0;

                    for(u32 plane_index = 0;
                            plane_index < world->plane_count;
                            ++plane_index)
                    {
                        plane *plane = world->planes + plane_index;
                        
                        //simd_v3 plane_normal = normal;
                        simd_v3 plane_normal = simd_v3_(plane->normal);
                        simd_f32 plane_d = simd_f32_(plane->d);

                        // NOTE(joon) : if the denominator is 0, it means that the ray is parallel to the plane
                        simd_f32 denom = dot(plane_normal, ray_dir);

                        simd_u32 denom_mask = compare_not_equal(denom, simd_f32_0);// if denom is not 0, 
                        
                        if(!all_lanes_zero(denom_mask))
                        {
                            // TODO(joon): We might just be dividing by 0... is this safe?
                            simd_f32 hit_t = (dot(-plane_normal, ray_origin) - plane_d)/denom;

                            // TODO(joon): denom_mask might not be necessary?
                            simd_u32 min_t_update_mask = denom_mask & compare_greater(hit_t, hit_t_threshold) & compare_less(hit_t, min_hit_t); 

                            if(!all_lanes_zero(min_t_update_mask))
                            {
                                // NOTE(joon): clear the values that will be overwritten by the new value, or them to overwrite
                                min_hit_t = overwrite(min_hit_t, min_t_update_mask, hit_t);
                                next_ray_origin = overwrite(next_ray_origin, min_t_update_mask, ray_origin + (hit_t*ray_dir));
                                next_normal = overwrite(next_normal, min_t_update_mask, plane_normal);

                                simd_u32 this_plane_mat_index = simd_u32_(plane->material_index);
                                hit_mat_index = overwrite(hit_mat_index, min_t_update_mask, this_plane_mat_index);
                            }
                        }
                    }

#if 1
                    for(u32 sphere_index = 0;
                            sphere_index < world->sphere_count;
                            ++sphere_index)
                    {
                        sphere *sphere = world->spheres + sphere_index;

                        simd_v3 sphere_center = simd_v3_(sphere->center);

                        simd_f32 sphere_radius_square = simd_f32_(sphere->radius * sphere->radius);

                        simd_v3 rel_ray_origin = ray_origin - sphere_center;
                        // NOTE(joon): We are using a simplified version of the solutions for the quadratic formula
                        // which is (-b +/ sqrt(b^2 - ac))/a, where b is half of the original b
                        simd_f32 a = dot(ray_dir, ray_dir);
                        simd_f32 b = dot(ray_dir, rel_ray_origin);
                        simd_f32 c = dot(rel_ray_origin, rel_ray_origin) - sphere_radius_square;

                        simd_f32 root_term = b*b - a*c;
                        simd_u32 root_term_is_positive_mask = compare_greater_equal(root_term, simd_f32_0);

                        if(!all_lanes_zero(root_term_is_positive_mask))
                        {
                            simd_f32 sqrt_root_term = sqrt(root_term);

                            simd_f32 minus_b = -b;
                            simd_f32 num0 = (-b - sqrt_root_term);
                            simd_f32 num1 = (-b + sqrt_root_term);

                            simd_f32 tn = (num0)/a;
                            simd_f32 tp = (num1)/a;

                            simd_f32 hit_t = tp;
                            // if tn is greater than the threshold & less than tp, we should replace tp with tn 
                            simd_u32 hit_t_overwrite_mask = compare_greater(tn, hit_t_threshold) & compare_less(tn, tp);
                            hit_t = overwrite(hit_t, hit_t_overwrite_mask, tn);

                            simd_u32 min_t_update_mask = compare_greater(hit_t, hit_t_threshold) & compare_less(hit_t, min_hit_t);
                            if(!all_lanes_zero(min_t_update_mask))
                            {
                                min_hit_t = overwrite(min_hit_t, min_t_update_mask, hit_t);
                                next_ray_origin = overwrite(next_ray_origin, min_t_update_mask, ray_origin + (hit_t*ray_dir));
                                next_normal = overwrite(next_normal, min_t_update_mask, next_ray_origin - sphere_center);

                                simd_u32 this_sphere_mat_index = simd_u32_(sphere->material_index);
                                hit_mat_index = overwrite(hit_mat_index, min_t_update_mask, this_sphere_mat_index);
                            }
                        }
                    }

#if 1
                    for(u32 triangle_index = 0;
                            triangle_index < world->triangle_count;
                            ++triangle_index)
                    {
                        triangle *triangle = world->triangles + triangle_index;

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
                        // TODO(joon): These ones don't need to be calculated as vector, we can just duplicate the result value 
                        // This might help to not over-populate the port that has vector arithmetic function

                        simd_v3 v0 = simd_v3_(triangle->v0);
                        simd_v3 v1 = simd_v3_(triangle->v1);
                        simd_v3 v2 = simd_v3_(triangle->v2);
                        
                        simd_v3 e1 = v1 - v0;
                        simd_v3 e2 = v2 - v0;

                        simd_v3 cross_ray_e2 = cross(ray_dir, e2);

                        simd_f32 det = dot(cross_ray_e2, e1);

                        // if the determinant is 0, it means the ray is parallel to the triangle
                        simd_u32 det_is_non_zero_mask = compare_not_equal(det, simd_f32_0);
                        if(!all_lanes_zero(det_is_non_zero_mask))
                        {
                            simd_v3 T = ray_origin - v0;
                            simd_v3 a = cross(T, e1);

                            simd_f32 hit_t = dot(a, e2) / det;
                            // NOTE(joon): barycentric coordinates of u and v, the last coordinate w = (1 - u - v)
                            simd_f32 u = dot(cross_ray_e2, T) / det;
                            simd_f32 v = dot(a, ray_dir) / det;

                            simd_u32 min_t_update_mask = compare_greater_equal(hit_t, hit_t_threshold) & 
                                                         compare_less(hit_t, min_hit_t) &
                                                         compare_greater_equal(u, simd_f32_0) & 
                                                         compare_greater_equal(v, simd_f32_0) & 
                                                         compare_less_equal(u+v, simd_f32_1);

                            if(!all_lanes_zero(min_t_update_mask))
                            {
                                // NOTE(joon): clear the values that will be overwritten by the new value, or them to overwrite
                                min_hit_t = overwrite(min_hit_t, min_t_update_mask, hit_t);
                                next_ray_origin = overwrite(next_ray_origin, min_t_update_mask, ray_origin + (hit_t*ray_dir));
                                next_normal = overwrite(next_normal, min_t_update_mask, normalize(cross(e1, e2)));

                                // TODO(joon): calculate normal based on the ray dir, so that the next normal is facing the incoming ray dir
                                // otherwise, the reflection vector will be totally busted?
                                simd_u32 this_triangle_mat_index = simd_u32_(triangle->material_index);
                                hit_mat_index = overwrite(hit_mat_index, min_t_update_mask, this_triangle_mat_index);
                            }
                        }
                    }
#endif
#endif
                    // TODO(joon): gatter / scatter?
                    material *lane_0_hit_material = world->materials + get_lane(hit_mat_index, 0);
                    material *lane_1_hit_material = world->materials + get_lane(hit_mat_index, 1);
                    material *lane_2_hit_material = world->materials + get_lane(hit_mat_index, 2);
                    material *lane_3_hit_material = world->materials + get_lane(hit_mat_index, 3);

                    simd_v3 hit_mat_emit_color = simd_v3_(lane_0_hit_material->emit_color,
                                                          lane_1_hit_material->emit_color, 
                                                          lane_2_hit_material->emit_color, 
                                                          lane_3_hit_material->emit_color);

                    simd_v3 hit_mat_reflection_color = simd_v3_(lane_0_hit_material->reflection_color, 
                                                                lane_1_hit_material->reflection_color, 
                                                                lane_2_hit_material->reflection_color, 
                                                                lane_3_hit_material->reflection_color);

                    simd_f32 hit_mat_reflectivity = simd_f32_(lane_0_hit_material->reflectivity, 
                                                              lane_1_hit_material->reflectivity, 
                                                              lane_2_hit_material->reflectivity, 
                                                              lane_3_hit_material->reflectivity);

                    bounced_ray_count += get_non_zero_lane_count_from_all_set_bit(is_ray_alive_mask);

                    // TODO(joon): cos?
                    // TODO(joon): make overwrite_plus function?
                    result_color = overwrite(result_color, is_ray_alive_mask, result_color + attenuation*hit_mat_emit_color); 

                    // NOTE(joon): update if the ray is still alive or dead
                    simd_u32 mat_mask = is_lane_non_zero(hit_mat_index);
                    is_ray_alive_mask = is_ray_alive_mask & mat_mask;

                    if(all_lanes_zero(is_ray_alive_mask))
                    {
                        break;
                    }
                    else
                    {
                        attenuation = overwrite(attenuation, is_ray_alive_mask, attenuation*hit_mat_reflection_color);

                        ray_origin = next_ray_origin;

                        next_normal = normalize(next_normal);
                        simd_v3 perfect_reflection = normalize(ray_dir - simd_f32_2*dot(ray_dir, next_normal)*next_normal);

                        simd_v3 random = simd_v3_(random_between_minus_1_1(series), random_between_minus_1_1(series), random_between_minus_1_1(series));

                        simd_v3 random_reflection = normalize(next_normal + random);

                        ray_dir = lerp(random_reflection, hit_mat_reflectivity, perfect_reflection);

                    }
                }
            }

            result_color /= simd_f32_ray_per_pixel_count;

            f32 result_r_f32 = linear_to_srgb(clamp01(add_all_lanes(result_color.r)));
            f32 result_g_f32 = linear_to_srgb(clamp01(add_all_lanes(result_color.g)));
            f32 result_b_f32 = linear_to_srgb(clamp01(add_all_lanes(result_color.b)));

            u32 result_r = round_r32_u32(255.0f*result_r_f32) << 16;
            u32 result_g = round_r32_u32(255.0f*result_g_f32) << 8;
            u32 result_b =  round_r32_u32(255.0f*result_b_f32) << 0;

            u32 result_pixel_color = 0xff << 24 |
                                    result_r |
                                    result_g |
                                    result_b;

            *pixel++ = result_pixel_color;
        }

        row += output_width;
    }

    result.bounced_ray_count = bounced_ray_count;

#if 0
    // NOTE(joon): Draw tile boundary
    {
        u32 *row = pixels + min_y*output_width + min_x;
        for(u32 y = min_y;
                y < one_past_max_y;
                ++y)
        {
            u32 *pixel = row;
            for(u32 x = min_x;
                    x < one_past_max_x;
                    ++x)
            {
                if(y == min_y || y == one_past_max_y-1 || x == min_x || x == one_past_max_x - 1)
                {
                    *pixel = 0xffff0000;
                }

                pixel++;
            }

            row += output_width;
        }
    }
#endif

    return result;
}

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
    // TODO(joon): Put this inside the raytracer input structure
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

        for(u32 x = min_x;
                x < one_past_max_x;
                ++x)
        {
            v3 result_color = v3_(0, 0, 0);

            for(u32 ray_per_pixel_index = 0;
                    ray_per_pixel_index < ray_per_pixel_count;
                    ++ray_per_pixel_index)
            {
                // NOTE(joon): These values are inside the loop because as we are casting multiple lights anyway,
                // we can slightly 'jitter' the ray direction to get the anti-aliasing effect 
                r32 film_x = 2.0f*((r32)x/(r32)output_width) - 1.0f;

                r32 jitter_x = x_per_pixel*random_between_0_1(&series);
                r32 jitter_y = x_per_pixel*random_between_0_1(&series);

                r32 offset_x = film_x + jitter_x;
                r32 offset_y = film_y + jitter_y;

                // axis(which are defined in world coordinate, so by multiplying them, we can get the world coodinates)
                v3 film_p = film_center + offset_y*half_film_height*camera_y_axis + offset_x*half_film_width*camera_x_axis;

                // TODO(joon) : later on, we need to make this to be random per ray per pixel!
                v3 ray_origin = camera_p;
                v3 ray_dir = film_p - camera_p;
                v3 attenuation = v3_(1, 1, 1);

                for(u32 bounce_index = 0;
                        bounce_index < 8;
                        ++bounce_index)
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

#if 1
                    for(u32 sphere_index = 0;
                            sphere_index < world->sphere_count;
                            ++sphere_index)
                    {
                        sphere *sphere = world->spheres + sphere_index;

                        ray_intersect_result intersect_result = ray_intersect_with_sphere(sphere->center, sphere->radius, ray_origin, ray_dir);
                        
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
#endif
                    bounced_ray_count++;
                    if(hit_mat_index)
                    {
                        material *hit_material = world->materials + hit_mat_index;
                        // TODO(joon): cos?
                        result_color += hadamard(attenuation, hit_material->emit_color); 
                        attenuation = hadamard(attenuation, hit_material->reflection_color);

                        ray_origin = next_ray_origin;

                        next_normal = normalize(next_normal);
                        v3 perfect_reflection = normalize(ray_dir -  2.0f*dot(ray_dir, next_normal)*next_normal);
                        v3 random_reflection = normalize(next_normal + v3_(random_between_minus_1_1(&series), random_between_minus_1_1(&series), random_between_minus_1_1(&series)));

                        ray_dir = lerp(random_reflection, hit_material->reflectivity, perfect_reflection);
                    }
                    else
                    {
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

#if 0
    // NOTE(joon): Draw tile boundary
    {
        u32 *row = pixels + min_y*output_width + min_x;
        for(u32 y = min_y;
                y < one_past_max_y;
                ++y)
        {
            u32 *pixel = row;
            for(u32 x = min_x;
                    x < one_past_max_x;
                    ++x)
            {
                if(y == min_y || y == one_past_max_y-1 || x == min_x || x == one_past_max_x - 1)
                {
                    *pixel = 0xffff0000;
                }

                pixel++;
            }

            row += output_width;
        }
    }
#endif

    return result;
}











