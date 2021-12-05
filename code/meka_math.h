#ifndef MEKA_MATH_H
#define MEKA_MATH_H

// NOTE(joon) : This file contains functions that requires math.h
// TODO(joon) : Someday we will get remove of math.h, too :)

#include <math.h>

inline v2
V2(r32 x, r32 y)
{
    v2 result = {};

    result.x = x;
    result.y = y;

    return result;
}

inline r32
length(v2 a)
{
    return sqrtf(a.x*a.x + a.y*a.y);
}


inline v2
V2i(i32 x, i32 y)
{
    v2 result = {};

    result.x = (r32)x;
    result.y = (r32)y;

    return result;
}

inline r32
length_square(v2 a)
{
    return a.x*a.x + a.y*a.y;
}


inline v2&
operator+=(v2 &v, v2 a)
{
    v.x += a.x;
    v.y += a.y;

    return v;
}

inline v2&
operator-=(v2 &v, v2 a)
{
    v.x -= a.x;
    v.y -= a.y;

    return v;
}

inline v2
operator-(v2 a)
{
    v2 result = {};

    result.x = -a.x;
    result.y = -a.y;

    return result;
}

inline v2
operator+(v2 a, v2 b)
{
    v2 result = {};

    result.x = a.x+b.x;
    result.y = a.y+b.y;

    return result;
}

inline v2
operator-(v2 a, v2 b)
{
    v2 result = {};

    result.x = a.x-b.x;
    result.y = a.y-b.y;

    return result;
}
inline v2
operator/(v2 a, r32 value)
{
    v2 result = {};

    result.x = a.x/value;
    result.y = a.y/value;

    return result;
}

inline v2
operator*(r32 value, v2 &a)
{
    v2 result = {};

    result.x = value*a.x;
    result.y = value*a.y;

    return result;
}

inline v3
V3(r32 x, r32 y, r32 z)
{
    v3 result = {};

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

inline r32
length(v3 a)
{
    return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
}

inline v3&
operator+=(v3 &v, v3 a)
{
    v.x += a.x;
    v.y += a.y;
    v.z += a.z;

    return v;
}

inline v3&
operator-=(v3 &v, v3 a)
{
    v.x -= a.x;
    v.y -= a.y;
    v.z -= a.z;

    return v;
}

inline v3
operator-(v3 a)
{
    v3 result = {};

    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;

    return result;
}

inline v3
operator+(v3 a, v3 b)
{
    v3 result = {};

    result.x = a.x+b.x;
    result.y = a.y+b.y;
    result.z = a.z+b.z;

    return result;
}

inline v3
operator-(v3 a, v3 b)
{
    v3 result = {};

    result.x = a.x-b.x;
    result.y = a.y-b.y;
    result.z = a.z-b.z;

    return result;
}
inline v3
operator/(v3 a, r32 value)
{
    v3 result = {};

    result.x = a.x/value;
    result.y = a.y/value;
    result.z = a.z/value;

    return result;
}


inline v3
operator*(r32 value, v3 a)
{
    v3 result = {};

    result.x = value*a.x;
    result.y = value*a.y;
    result.z = value*a.z;

    return result;
}

// RHS!!!!
// NOTE(joon) : This assumes the vectors ordered counter clockwisely
inline v3
cross(v3 a, v3 b)
{
    v3 result = {};

    result.x = a.y*b.z - b.y*a.z;
    result.y = -(a.x*b.z - b.x*a.z);
    result.z = a.x*b.y - b.x*a.y;

    return result;
}

inline r32
length_square(v3 a)
{
    return a.x*a.x + a.y*a.y + a.z*a.z;
}


inline v3
normalize(v3 a)
{
    return a/length(a);
}

inline b32
is_normalized(v3 a)
{
    return (a.x <= 1.0f && a.y <= 1.0f && a.z <= 1.0f);
}

inline r32
dot(v3 a, v3 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline v3
hadamard(v3 a, v3 b)
{
    return V3(a.x*b.x, a.y*b.y, a.z*b.z);
}

inline v4
V4(r32 x, r32 y, r32 z, r32 w)
{
    v4 result = {};
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

inline v4
V4(v3 xyz, r32 w)
{
    v4 result = {};
    result.xyz = xyz;
    result.w = w;

    return result;
}

inline r32
length_square(v4 a)
{
    return a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w;
}


inline v4
operator/(v4 a, r32 value)
{
    v4 result = {};

    result.x = a.x/value;
    result.y = a.y/value;
    result.z = a.z/value;
    result.w = a.w/value;

    return result;
}

inline r32
length(v4 a)
{
    return sqrtf(length_square(a));
}

inline v4
normalize(v4 a)
{
    return a/length(a);
}

inline v4
operator+(v4 &a, v4 &b)
{
    v4 result = {};

    result.x = a.x+b.x;
    result.y = a.y+b.y;
    result.z = a.z+b.z;
    result.w = a.w+b.w;

    return result;
}

inline v4
operator-(v4 &a, v4 &b)
{
    v4 result = {};

    result.x = a.x-b.x;
    result.y = a.y-b.y;
    result.z = a.z-b.z;
    result.w = a.w-b.w;

    return result;
}

inline v4
operator*(r32 value, v4 &a)
{
    v4 result = {};

    result.x = value*a.x;
    result.y = value*a.y;
    result.z = value*a.z;
    result.w = value*a.w;

    return result;
}

// return identity matrix
inline m4
M4()
{
    m4 result = {};
    result.column[0].e[0] = 1.0f;
    result.column[1].e[1] = 1.0f;
    result.column[2].e[2] = 1.0f;
    result.column[3].e[3] = 1.0f;

    return result;
}

inline m4
M4(r32 e00, r32 e01, r32 e02, r32 e03,
    r32 e10, r32 e11, r32 e12, r32 e13,
    r32 e20, r32 e21, r32 e22, r32 e23,
    r32 e30, r32 e31, r32 e32, r32 e33)
{
    m4 result = {};

    result.column[0].e[0] = e00;
    result.column[0].e[1] = e10;
    result.column[0].e[2] = e20;
    result.column[0].e[3] = e30;

    result.column[1].e[0] = e01;
    result.column[1].e[1] = e11;
    result.column[1].e[2] = e21;
    result.column[1].e[3] = e31;

    result.column[2].e[0] = e02;
    result.column[2].e[1] = e12;
    result.column[2].e[2] = e22;
    result.column[2].e[3] = e32;

    result.column[3].e[0] = e03;
    result.column[3].e[1] = e13;
    result.column[3].e[2] = e23;
    result.column[3].e[3] = e33;

    return result;
}

inline m4
operator+(m4 &a, m4 &b)
{
    m4 result = {};

    result.column[0] = a.column[0] + b.column[0];
    result.column[1] = a.column[1] + b.column[1];
    result.column[2] = a.column[2] + b.column[2];
    result.column[3] = a.column[3] + b.column[3];

    return result;
}

inline m4
operator-(m4 &a, m4 &b)
{
    m4 result = {};

    result.column[0] = a.column[0] - b.column[0];
    result.column[1] = a.column[1] - b.column[1];
    result.column[2] = a.column[2] - b.column[2];
    result.column[3] = a.column[3] - b.column[3];

    return result;
}

inline m4
operator*(m4 a, m4 b)
{
    m4 result = {};

    result.column[0].e[0] = a.column[0].e[0]*b.column[0].e[0] +
                            a.column[1].e[0]*b.column[0].e[1] +
                            a.column[2].e[0]*b.column[0].e[2] +
                            a.column[3].e[0]*b.column[0].e[3];
    result.column[0].e[1] = a.column[0].e[1]*b.column[0].e[0] +
                            a.column[1].e[1]*b.column[0].e[1] +
                            a.column[2].e[1]*b.column[0].e[2] +
                            a.column[3].e[1]*b.column[0].e[3];
    result.column[0].e[2] = a.column[0].e[2]*b.column[0].e[0] +
                            a.column[1].e[2]*b.column[0].e[1] +
                            a.column[2].e[2]*b.column[0].e[2] +
                            a.column[3].e[2]*b.column[0].e[3];
    result.column[0].e[3] = a.column[0].e[3]*b.column[0].e[0] +
                            a.column[1].e[3]*b.column[0].e[1] +
                            a.column[2].e[3]*b.column[0].e[2] +
                            a.column[3].e[3]*b.column[0].e[3];

    result.column[1].e[0] = a.column[0].e[0]*b.column[1].e[0] +
                            a.column[1].e[0]*b.column[1].e[1] +
                            a.column[2].e[0]*b.column[1].e[2] +
                            a.column[3].e[0]*b.column[1].e[3];
    result.column[1].e[1] = a.column[0].e[1]*b.column[1].e[0] +
                            a.column[1].e[1]*b.column[1].e[1] +
                            a.column[2].e[1]*b.column[1].e[2] +
                            a.column[3].e[1]*b.column[1].e[3];
    result.column[1].e[2] = a.column[0].e[2]*b.column[1].e[0] +
                            a.column[1].e[2]*b.column[1].e[1] +
                            a.column[2].e[2]*b.column[1].e[2] +
                            a.column[3].e[2]*b.column[1].e[3];
    result.column[1].e[3] = a.column[0].e[3]*b.column[1].e[0] +
                            a.column[1].e[3]*b.column[1].e[1] +
                            a.column[2].e[3]*b.column[1].e[2] +
                            a.column[3].e[3]*b.column[1].e[3];

    result.column[2].e[0] = a.column[0].e[0]*b.column[2].e[0] +
                            a.column[1].e[0]*b.column[2].e[1] +
                            a.column[2].e[0]*b.column[2].e[2] +
                            a.column[3].e[0]*b.column[2].e[3];
    result.column[2].e[1] = a.column[0].e[1]*b.column[2].e[0] +
                            a.column[1].e[1]*b.column[2].e[1] +
                            a.column[2].e[1]*b.column[2].e[2] +
                            a.column[3].e[1]*b.column[2].e[3];
    result.column[2].e[2] = a.column[0].e[2]*b.column[2].e[0] +
                            a.column[1].e[2]*b.column[2].e[1] +
                            a.column[2].e[2]*b.column[2].e[2] +
                            a.column[3].e[2]*b.column[2].e[3];
    result.column[2].e[3] = a.column[0].e[3]*b.column[2].e[0] +
                            a.column[1].e[3]*b.column[2].e[1] +
                            a.column[2].e[3]*b.column[2].e[2] +
                            a.column[3].e[3]*b.column[2].e[3];

    result.column[3].e[0] = a.column[0].e[0]*b.column[3].e[0] +
                            a.column[1].e[0]*b.column[3].e[1] +
                            a.column[2].e[0]*b.column[3].e[2] +
                            a.column[3].e[0]*b.column[3].e[3];
    result.column[3].e[1] = a.column[0].e[1]*b.column[3].e[0] +
                            a.column[1].e[1]*b.column[3].e[1] +
                            a.column[2].e[1]*b.column[3].e[2] +
                            a.column[3].e[1]*b.column[3].e[3];
    result.column[3].e[2] = a.column[0].e[2]*b.column[3].e[0] +
                            a.column[1].e[2]*b.column[3].e[1] +
                            a.column[2].e[2]*b.column[3].e[2] +
                            a.column[3].e[2]*b.column[3].e[3];
    result.column[3].e[3] = a.column[0].e[3]*b.column[3].e[0] +
                            a.column[1].e[3]*b.column[3].e[1] +
                            a.column[2].e[3]*b.column[3].e[2] +
                            a.column[3].e[3]*b.column[3].e[3];


    return result;
}

inline m4
operator*(r32 value, m4 &b)
{
    m4 result = {};
    result.column[0] = value*b.column[0];
    result.column[1] = value*b.column[1];
    result.column[2] = value*b.column[2];
    result.column[3] = value*b.column[3];

    return result;
}

inline v4
operator*(m4 m, v4 v)
{
    v4 result = {};
    result.x = m.column[0].e[0]*v.x + 
            m.column[1].e[0]*v.y +
            m.column[2].e[0]*v.z +
            m.column[3].e[0]*v.w;

    result.y = m.column[0].e[1]*v.x + 
            m.column[1].e[1]*v.y +
            m.column[2].e[1]*v.z +
            m.column[3].e[1]*v.w;

    result.z = m.column[0].e[2]*v.x + 
            m.column[1].e[2]*v.y +
            m.column[2].e[2]*v.z +
            m.column[3].e[2]*v.w;

    result.w = m.column[0].e[3]*v.x + 
            m.column[1].e[3]*v.y +
            m.column[2].e[3]*v.z +
            m.column[3].e[3]*v.w;

    return result;
}



inline m4
scale_m4(v3 scale)
{
    m4 result = M4();

    result.column[0].e[0] *= scale.x;
    result.column[1].e[1] *= scale.y;
    result.column[2].e[2] *= scale.z;
    
    return result;
}

inline m4
scale_m4(r32 x, r32 y, r32 z)
{
    return scale_m4(V3(x, y, z));
}

inline m4
translate_m4(v3 translate)
{
    m4 result = M4();

    result.column[3].xyz = translate;

    return result;
}

inline m4
translate_m4(r32 x, r32 y, r32 z)
{
    return translate_m4(V3(x, y, z));
}

/*
 * Rotation matrix along one axis using Quarternion(&q is a unit quarternion)
 * 1-2y^2-2z^2      2xy-2sz      2xz+2sy
 * 2xy+2sz          1-2x^2-2z^2  2yz-2sx
 * 2xz-2sy          2yz+2sx      1-2x^2-2y^2
 *
*/
inline v3
QuarternionRotation(v3 axisVector, r32 rad, v3 vectorToRotate)
{
    v3 result = {};

    // Quarternion q = q0 + q1*i + q2*j + q3*k, or s + xi + yj + zk
    r32 q0 = cosf(rad/2);
    r32 q1 = axisVector.x*sinf(rad/2);
    r32 q2 = axisVector.y*sinf(rad/2);
    r32 q3 = axisVector.z*sinf(rad/2);

    r32 m00 = 1.0f - 2*q2*q2 - 2*q3*q3;
    r32 m01 = 2*q1*q2 - 2*q0*q3;
    r32 m02 = 2*q1*q3 + 2*q0*q2;
    r32 m10 = 2*q1*q2 + 2*q0*q3;
    r32 m11 = 1.0f - 2*q1*q1 - 2*q3*q3;
    r32 m12 = 2*q2*q3 - 2*q0*q1;
    r32 m20 = 2*q1*q3 - 2*q0*q2;
    r32 m21 = 2*q2*q3 + 2*q0*q1;
    r32 m22 = 1 - 2*q1*q1 - 2*q2*q2;

    result.x = m00*vectorToRotate.x + m01*vectorToRotate.y + m02*vectorToRotate.z;
    result.y = m10*vectorToRotate.x + m11*vectorToRotate.y + m12*vectorToRotate.z;
    result.z = m20*vectorToRotate.x + m21*vectorToRotate.y + m22*vectorToRotate.z;

    return result;
}

inline m4
QuarternionRotationM4(v3 axisVector, r32 rad)
{
    m4 result = {};

    // Quarternion q = q0 + q1*i + q2*j + q3*k, or s + xi + yj + zk
    r32 q0 = cosf(rad/2);
    r32 q1 = axisVector.x*sinf(rad/2);
    r32 q2 = axisVector.y*sinf(rad/2);
    r32 q3 = axisVector.z*sinf(rad/2);

    r32 m00 = 1.0f - 2*q2*q2 - 2*q3*q3;
    r32 m01 = 2*q1*q2 - 2*q0*q3;
    r32 m02 = 2*q1*q3 + 2*q0*q2;
    r32 m10 = 2*q1*q2 + 2*q0*q3;
    r32 m11 = 1.0f - 2*q1*q1 - 2*q3*q3;
    r32 m12 = 2*q2*q3 - 2*q0*q1;
    r32 m20 = 2*q1*q3 - 2*q0*q2;
    r32 m21 = 2*q2*q3 + 2*q0*q1;
    r32 m22 = 1 - 2*q1*q1 - 2*q2*q2;

    result.column[0] = V4(m00, m10, m20, 0);
    result.column[1] = V4(m01, m11, m21, 0);
    result.column[2] = V4(m02, m12, m22, 0);
    result.column[3] = V4(0, 0, 0, 1);

    return result;
}


// calculate  quarternion along x axis(1,0,0)
inline v3
QuarternionRotationV100(r32 rad, v3 vectorToRotate)
{
    v3 result = {};

    r32 cos = cosf(rad/2);
    r32 sin = sinf(rad/2);

    r32 a = 1-2.0f*sin*sin;
    r32 b = 2*cos*sin;

    result.x = vectorToRotate.x;
    result.y = a*vectorToRotate.y - b*vectorToRotate.z;
    result.z = b*vectorToRotate.y + a*vectorToRotate.z;

    return result;
}

// calculate  quarternion along yaxis(0,1,0)
inline v3
QuarternionRotationV010(r32 rad, v3 vectorToRotate)
{
    v3 result = {};

    r32 cos = cosf(rad/2);
    r32 sin = sinf(rad/2);

    r32 a = 1 - 2.0f*sin*sin;
    r32 b = 2*cos*sin;

    result.x = a*vectorToRotate.x + b*vectorToRotate.z;
    result.y = vectorToRotate.y;
    result.z = -b*vectorToRotate.x + a*vectorToRotate.z;

    return result;
}

// calculate  quarternion along yaxis(0,0,-1)
inline v3
QuarternionRotationV001(r32 rad, v3 vectorToRotate)
{
    v3 result = {};

    r32 cos = cosf(rad/2);
    r32 sin = sinf(rad/2);

    r32 a = 1 - 2.0f*sin*sin;
    r32 b = 2*cos*sin;

    result.x = a*vectorToRotate.x - b*vectorToRotate.y;
    result.y = b*vectorToRotate.x + a*vectorToRotate.y;
    result.z = vectorToRotate.z;

    return result;
}

// NOTE/Joon: RHS/initial view direction is 1, 0, 0(in world space)
inline m4
world_to_camera(v3 camera_world_p, r32 along_x, r32 along_y, r32 along_z)
{
    // quarternion does not guarantee the rotation order independency(this buggd me quite a while)
    // so in here, we will assume that the order of rotation is always x->y->z
    // as long as we are begin consistent, we should be fine.
    m4 quarternion = QuarternionRotationM4(V3(0, 0, 1), along_z)*QuarternionRotationM4(V3(0, 1, 0), along_y)*QuarternionRotationM4(V3(1, 0, 0), along_x);
    v3 camera_x_axis = (quarternion*V4(1, 0, 0, 0)).xyz;
    v3 camera_y_axis = (quarternion*V4(0, 1, 0, 0)).xyz;
    v3 camera_z_axis = (quarternion*V4(0, 0, 1, 0)).xyz;

    // NOTE(joon): Identical with projecting the world space coordinate onto the camera vectors
    m4 rotation = {};
    rotation.column[0] = V4(camera_x_axis.x, camera_y_axis.x, camera_z_axis.x, 0);
    rotation.column[1] = V4(camera_x_axis.y, camera_y_axis.y, camera_z_axis.y, 0);
    rotation.column[2] = V4(camera_x_axis.z, camera_y_axis.z, camera_z_axis.z, 0);
    rotation.column[3] = V4(0, 0, 0, 1);

    m4 translation = translate_m4(-camera_world_p.x, -camera_world_p.y, -camera_world_p.z);

    return rotation*translation;
} 

// NOTE(joon): Assumes that the initial lookat dir is always (1, 0, 0) in world coordinate
// this is required because we the only thing we have is the angles
inline v3
camera_to_world_v100(r32 along_x, r32 along_y, r32 along_z)
{
    m4 quarternion = QuarternionRotationM4(V3(0, 0, 1), along_z)*QuarternionRotationM4(V3(0, 1, 0), along_y)*QuarternionRotationM4(V3(1, 0, 0), along_x);
    v3 camera_x_axis = (quarternion*V4(1, 0, 0, 0)).xyz;

    return camera_x_axis;
}

inline m4
camera_rhs_to_lhs()
{
    m4 result = {};

    result.column[0] = V4(0, 0, 1, 0);
    result.column[1] = V4(-1, 0, 0, 0);
    result.column[2] = V4(0, 1, 0, 0);
    result.column[3] = V4(0, 0, 0, 1);

    return result;
}

inline b32
clip_space_top_is_one(void)
{
    b32 result = false;
#if MEKA_METAL || MEKA_OPENGL
    result = true;
#elif MEKA_VULKAN
    result = false;
#endif

    return result;
}

// NOTE/Joon : This assumes that the window width is always 1m
inline m4
projection(r32 focal_length, r32 aspect_ratio_width_over_height, r32 near, r32 far)
{
    m4 result = {};

    r32 c = clip_space_top_is_one() ? 1.f : 0.f; 

    result.column[0] = V4(focal_length, 0, 0, 0);
    result.column[1] = V4(0, focal_length*aspect_ratio_width_over_height, 0, 0);
    result.column[2] = V4(0, 0, c*(near+far)/(far-near), 1);
    result.column[3] = V4(0, 0, (-2.0f*far*near)/(far-near), 0);

    return result;
}

// r = frustumHalfWidth
// t = frustumHalfHeight
// n = near plane z value
// f = far plane z value
inline m4
symmetric_projection(r32 r, r32 t, r32 n, r32 f)
{
    m4 result = {};

    r32 c = clip_space_top_is_one() ? 1.f : 0.f; 

    // IMPORTANT : In opengl, t corresponds to 1 -> column[1][1] = n/t
    // while in vulkan, t corresponds to -1 -> columm[1][1] = -n/t
    result.column[0] = V4(n/r, 0, 0, 0);
    result.column[1] = V4(0, (n/t), 0, 0);
    result.column[2] = V4(0, 0, c*(n+f)/(n-f), -1);
    result.column[3] = V4(0, 0, (2.0f*f*n)/(n-f), 0);

    return result;
}

inline r32 
Clamp(r32 min, r32 value, r32 max)
{
    r32 result = value;
    if(result < min)
    {
        result = min;
    }
    if(result > max)
    {
        result = max;
    }

    return result;
}
inline r32 
Clamp01(r32 value)
{
    return Clamp(0.0f, value, 1.0f);
}


inline u32
Clamp(u32 min, u32 value, u32 max)
{
    u32 result = value;
    if(result < min)
    {
        result = min;
    }
    if(result > max)
    {
        result = max;
    }

    return result;
}

inline i32
Clamp(i32 min, i32 value, i32 max)
{
    i32 result = value;
    if(result < min)
    {
        result = min;
    }
    if(result > max)
    {
        result = max;
    }

    return result;
}

inline r32
Sin(r32 rad)
{
    return sinf(rad);
}

inline r32
Cos(r32 rad)
{
    return cosf(rad);
}


#endif

