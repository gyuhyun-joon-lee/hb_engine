#ifndef MEKA_MATH_H
#define MEKA_MATH_H

// NOTE(joon) : This file contains functions that requires math.h
// TODO(joon) : Someday we will get remove of math.h, too :)
#include <math.h>

// TODO(joon) This function is not actually 'safe'...
// need better name here
inline b32
compare_with_epsilon(f32 a, f32 b, f32 epsilon = 0.00005f)
{
    b32 result = true;

    f32 d = a - b;

    if(d < -epsilon || d > epsilon)
    {
        result = false;
    }

    return result;
}

inline f32
safe_ratio(f32 nom, f32 denom)
{
    f32 result = Flt_Max;

    f32 epsilon = 0.000001f;
    if(denom < -epsilon || denom > epsilon)
    {
        result = nom / denom;
    }

    return result;
}

inline f32
square(f32 value)
{
    return value*value;
}

inline v2
V2(f32 x, f32 y)
{
    v2 result = {};

    result.x = x;
    result.y = y;

    return result;
}

inline f32
length(v2 a)
{
    return sqrtf(a.x*a.x + a.y*a.y);
}

inline v2
V2i(i32 x, i32 y)
{
    v2 result = {};

    result.x = (f32)x;
    result.y = (f32)y;

    return result;
}

inline f32
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
operator/(v2 a, f32 value)
{
    v2 result = {};

    result.x = a.x/value;
    result.y = a.y/value;

    return result;
}

inline v2
operator*(f32 value, v2 &a)
{
    v2 result = {};

    result.x = value*a.x;
    result.y = value*a.y;

    return result;
}

inline v3
V3()
{
    v3 result = {};

    return result;
}

inline v3
V3(f32 x, f32 y, f32 z)
{
    v3 result = {};

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

inline f32
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
operator/(v3 a, f32 value)
{
    v3 result = {};

    result.x = a.x/value;
    result.y = a.y/value;
    result.z = a.z/value;

    return result;
}

inline v3&
operator/=(v3 &a, f32 value)
{
    a.x /= value;
    a.y /= value;
    a.z /= value;

    return a;
}

inline v3&
operator*=(v3 &a, f32 value)
{
    a.x *= value;
    a.y *= value;
    a.z *= value;

    return a;
}

inline v3
operator*(f32 value, v3 a)
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
    result.y = b.x*a.z - a.x*b.z;
    result.z = a.x*b.y - b.x*a.y;

    return result;
}

inline f32
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

inline f32
dot(v3 a, v3 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline v3
hadamard(v3 a, v3 b)
{
    return V3(a.x*b.x, a.y*b.y, a.z*b.z);
}

inline v3
gather_min_elements(v3 a, v3 b)
{
    v3 result = {};

    result.x = minimum(a.x, b.x);
    result.y = minimum(a.y, b.y);
    result.z = minimum(a.z, b.z);

    return result;
}

inline v3
gather_max_elements(v3 a, v3 b)
{
    v3 result = {};

    result.x = maximum(a.x, b.x);
    result.y = maximum(a.y, b.y);
    result.z = maximum(a.z, b.z);

    return result;
}

inline v4
V4(f32 x, f32 y, f32 z, f32 w)
{
    v4 result = {};
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

inline v4
V4(v3 xyz, f32 w)
{
    v4 result = {};
    result.xyz = xyz;
    result.w = w;

    return result;
}

inline f32
length_square(v4 a)
{
    return a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w;
}

inline f32
dot(v4 a, v4 b)
{
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    return result;
}

inline v4
operator/(v4 a, f32 value)
{
    v4 result = {};

    result.x = a.x/value;
    result.y = a.y/value;
    result.z = a.z/value;
    result.w = a.w/value;

    return result;
}

inline f32
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
operator*(f32 value, v4 &a)
{
    v4 result = {};

    result.x = value*a.x;
    result.y = value*a.y;
    result.z = value*a.z;
    result.w = value*a.w;

    return result;
}

inline v4&
operator*=(v4 &a, f32 value)
{
    a.x *= value;
    a.y *= value;
    a.z *= value;
    a.w *= value;

    return a;
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

inline m3x3
M3x3(f32 e00, f32 e01, f32 e02,
    f32 e10, f32 e11, f32 e12, 
    f32 e20, f32 e21, f32 e22)
{
    m3x3 result = {};
    result.rows[0] = {e00, e01, e02};
    result.rows[1] = {e10, e11, e12};
    result.rows[2] = {e20, e21, e22};

    return result;
}

// NOTE(joon) returns identity matrix
inline m3x3
M3x3(void)
{
    m3x3 result = {};

    result.e[0][0] = 1.0f;
    result.e[1][1] = 1.0f;
    result.e[2][2] = 1.0f;

    return result;
}

inline m3x3
operator *(m3x3 a, m3x3 b)
{
    m3x3 result = {}; 
    result.e[0][0] = a.e[0][0]*b.e[0][0] + a.e[0][1]*b.e[1][0] + a.e[0][2]*b.e[2][0];
    result.e[0][1] = a.e[0][0]*b.e[0][1] + a.e[0][1]*b.e[1][1] + a.e[0][2]*b.e[2][1];
    result.e[0][2] = a.e[0][0]*b.e[0][2] + a.e[0][1]*b.e[1][2] + a.e[0][2]*b.e[2][2];

    result.e[1][0] = a.e[1][0]*b.e[0][0] + a.e[1][1]*b.e[1][0] + a.e[1][2]*b.e[2][0];
    result.e[1][1] = a.e[1][0]*b.e[0][1] + a.e[1][1]*b.e[1][1] + a.e[1][2]*b.e[2][1];
    result.e[1][2] = a.e[1][0]*b.e[0][2] + a.e[1][1]*b.e[1][2] + a.e[1][2]*b.e[2][2];

    result.e[2][0] = a.e[2][0]*b.e[0][0] + a.e[2][1]*b.e[1][0] + a.e[2][2]*b.e[2][0];
    result.e[2][1] = a.e[2][0]*b.e[0][1] + a.e[2][1]*b.e[1][1] + a.e[2][2]*b.e[2][1];
    result.e[2][2] = a.e[2][0]*b.e[0][2] + a.e[2][1]*b.e[1][2] + a.e[2][2]*b.e[2][2];

    return result;
}

inline m3x3
operator +(m3x3 a, m3x3 b)
{
    m3x3 result = {};

    result.rows[0] = a.rows[0] + b.rows[0];
    result.rows[1] = a.rows[1] + b.rows[1];
    result.rows[2] = a.rows[2] + b.rows[2];

    return result;
}

inline m3x3
operator -(m3x3 a, m3x3 b)
{
    m3x3 result = {};

    result.rows[0] = a.rows[0] - b.rows[0];
    result.rows[1] = a.rows[1] - b.rows[1];
    result.rows[2] = a.rows[2] - b.rows[2];

    return result;
}

inline m3x3
transpose(m3x3 m)
{
    m3x3 result = {};

    for(u32 column = 0;
            column < 3;
            ++column)
    {
        for(u32 row = 0;
                row < 3;
                ++row)
        {
            result.e[row][column] = m.e[column][row];
        }
    }

    return result;
}

inline v3
operator *(m3x3 a, v3 b)
{
    v3 result = {};

    result.x = dot(a.rows[0], b);
    result.y = dot(a.rows[1], b);
    result.z = dot(a.rows[2], b);

    return result;
}

inline m3x3&
operator *=(m3x3 &m, f32 value)
{
    m.rows[0] *= value;
    m.rows[1] *= value;
    m.rows[2] *= value;

    return m;
}

inline m3x3
inverse(m3x3 m)
{
    m3x3 result = {};
    f32 det = m.e[0][0]*m.e[1][1]*m.e[2][2] + 
              m.e[1][0]*m.e[2][1]*m.e[0][2] + 
              m.e[2][0]*m.e[0][1]*m.e[1][2] - 
              m.e[0][0]*m.e[2][1]*m.e[1][2] - 
              m.e[2][0]*m.e[1][1]*m.e[0][2] - 
              m.e[1][0]*m.e[0][1]*m.e[2][2];

    if(!compare_with_epsilon(det, 0.0f))
    {
        result.e[0][0] = m.e[1][1]*m.e[2][2] - m.e[1][2]*m.e[2][1];
        result.e[0][1] = m.e[0][2]*m.e[2][1] - m.e[0][1]*m.e[2][2];
        result.e[0][2] = m.e[0][1]*m.e[1][2] - m.e[0][2]*m.e[1][1];

        result.e[1][0] = m.e[1][2]*m.e[2][0] - m.e[1][0]*m.e[2][2];
        result.e[1][1] = m.e[0][0]*m.e[2][2] - m.e[0][2]*m.e[2][0];
        result.e[1][2] = m.e[0][2]*m.e[1][0] - m.e[0][0]*m.e[1][2];

        result.e[2][0] = m.e[1][0]*m.e[2][1] - m.e[1][1]*m.e[2][0];
        result.e[2][1] = m.e[0][1]*m.e[2][0] - m.e[0][0]*m.e[2][1];
        result.e[2][2] = m.e[0][0]*m.e[1][1] - m.e[0][1]*m.e[1][0];

        result *= (1.0f/det);
    }

    return result;
}



// NOTE(joon) rotate along x axis
inline m3x3
x_rotate(f32 rad)
{
    // same as 2d rotation, without x value changing
    // cos -sin
    // sin cos
    f32 cos = cosf(rad);
    f32 sin = sinf(rad);

    m3x3 result = M3x3();
    result.e[1][1] = cos;
    result.e[1][2] = -sin;

    result.e[2][1] = sin;
    result.e[2][2] = cos;

    return result;
}

// NOTE(joon) rotate along y axis
inline m3x3
y_rotate(f32 rad)
{
    // same as 2d rotation, without y value changing
    // cos -sin
    // sin cos
    f32 cos = cosf(rad);
    f32 sin = sinf(rad);

    m3x3 result = 
    {
        {
            cos, 0.0f, sin,
            0.0f, 1.0f, 0.0f,
            -sin, 0.0f, cos
        }
    };

    return result;
}

// NOTE(joon) rotate along z axis
inline m3x3 
z_rotate(f32 rad)
{
    // same as 2d rotation, without x value changing
    // cos -sin
    // sin cos
    f32 cos = cosf(rad);
    f32 sin = sinf(rad);

    m3x3 result = M3x3();
    result.e[0][0] = cos;
    result.e[0][1] = -sin;

    result.e[1][0] = sin;
    result.e[1][1] = cos;

    return result;
}

inline m4x4
M4x4(f32 e00, f32 e01, f32 e02, f32 e03,
    f32 e10, f32 e11, f32 e12, f32 e13,
    f32 e20, f32 e21, f32 e22, f32 e23,
    f32 e30, f32 e31, f32 e32, f32 e33)
{
    m4x4 result = {};
    result.rows[0] = {e00, e01, e02, e03};
    result.rows[1] = {e10, e11, e12, e13};
    result.rows[2] = {e20, e21, e22, e23};
    result.rows[3] = {e30, e31, e32, e33};

    return result;
}

inline m4x4
M4x4(void)
{
    m4x4 result = {};

    result.e[0][0] = 1.0f;
    result.e[1][1] = 1.0f;
    result.e[2][2] = 1.0f;
    result.e[3][3] = 1.0f;

    return result;
}

inline m4x4
M4x4(m3x3 m)
{
    m4x4 result = {};

    result.rows[0] = V4(m.rows[0], 0.0f);
    result.rows[1] = V4(m.rows[1], 0.0f);
    result.rows[2] = V4(m.rows[2], 0.0f);

    return result;
}

inline m4x4
operator *(m4x4 a, m4x4 b)
{
    m4x4 result = {}; 
    result.e[0][0] = a.e[0][0]*b.e[0][0] + a.e[0][1]*b.e[1][0] + a.e[0][2]*b.e[2][0] + a.e[0][3]*b.e[3][0];
    result.e[0][1] = a.e[0][0]*b.e[0][1] + a.e[0][1]*b.e[1][1] + a.e[0][2]*b.e[2][1] + a.e[0][3]*b.e[3][1];
    result.e[0][2] = a.e[0][0]*b.e[0][2] + a.e[0][1]*b.e[1][2] + a.e[0][2]*b.e[2][2] + a.e[0][3]*b.e[3][2];
    result.e[0][3] = a.e[0][0]*b.e[0][3] + a.e[0][1]*b.e[1][3] + a.e[0][2]*b.e[2][3] + a.e[0][3]*b.e[3][3];

    result.e[1][0] = a.e[1][0]*b.e[0][0] + a.e[1][1]*b.e[1][0] + a.e[1][2]*b.e[2][0] + a.e[1][3]*b.e[3][0];
    result.e[1][1] = a.e[1][0]*b.e[0][1] + a.e[1][1]*b.e[1][1] + a.e[1][2]*b.e[2][1] + a.e[1][3]*b.e[3][1];
    result.e[1][2] = a.e[1][0]*b.e[0][2] + a.e[1][1]*b.e[1][2] + a.e[1][2]*b.e[2][2] + a.e[1][3]*b.e[3][2];
    result.e[1][3] = a.e[1][0]*b.e[0][3] + a.e[1][1]*b.e[1][3] + a.e[1][2]*b.e[2][3] + a.e[1][3]*b.e[3][3];

    result.e[2][0] = a.e[2][0]*b.e[0][0] + a.e[2][1]*b.e[1][0] + a.e[2][2]*b.e[2][0] + a.e[2][3]*b.e[3][0];
    result.e[2][1] = a.e[2][0]*b.e[0][1] + a.e[2][1]*b.e[1][1] + a.e[2][2]*b.e[2][1] + a.e[2][3]*b.e[3][1];
    result.e[2][2] = a.e[2][0]*b.e[0][2] + a.e[2][1]*b.e[1][2] + a.e[2][2]*b.e[2][2] + a.e[2][3]*b.e[3][2];
    result.e[2][3] = a.e[2][0]*b.e[0][3] + a.e[2][1]*b.e[1][3] + a.e[2][2]*b.e[2][3] + a.e[2][3]*b.e[3][3];

    result.e[3][0] = a.e[3][0]*b.e[0][0] + a.e[3][1]*b.e[1][0] + a.e[3][2]*b.e[2][0] + a.e[3][3]*b.e[3][0];
    result.e[3][1] = a.e[3][0]*b.e[0][1] + a.e[3][1]*b.e[1][1] + a.e[3][2]*b.e[2][1] + a.e[3][3]*b.e[3][1];
    result.e[3][2] = a.e[3][0]*b.e[0][2] + a.e[3][1]*b.e[1][2] + a.e[3][2]*b.e[2][2] + a.e[3][3]*b.e[3][2];
    result.e[3][3] = a.e[3][0]*b.e[0][3] + a.e[3][1]*b.e[1][3] + a.e[3][2]*b.e[2][3] + a.e[3][3]*b.e[3][3];

    return result;
}

inline m4x4
operator +(m4x4 a, m4x4 b)
{
    m4x4 result = {};

    result.rows[0] = a.rows[0] + b.rows[0];
    result.rows[1] = a.rows[1] + b.rows[1];
    result.rows[2] = a.rows[2] + b.rows[2];
    result.rows[3] = a.rows[3] + b.rows[3];

    return result;
}

inline m4x4
operator -(m4x4 a, m4x4 b)
{
    m4x4 result = {};

    result.rows[0] = a.rows[0] - b.rows[0];
    result.rows[1] = a.rows[1] - b.rows[1];
    result.rows[2] = a.rows[2] - b.rows[2];
    result.rows[3] = a.rows[3] - b.rows[3];

    return result;
}

inline m4x4
transpose(m4x4 m)
{
    m4x4 result = {};

    for(u32 column = 0;
            column < 4;
            ++column)
    {
        for(u32 row = 0;
                row < 4;
                ++row)
        {
            result.e[row][column] = m.e[column][row];
        }
    }

    return result;
}

inline v4
operator *(m4x4 a, v4 b)
{
    v4 result = {};

    result.x = dot(a.rows[0], b);
    result.y = dot(a.rows[1], b);
    result.z = dot(a.rows[2], b);
    result.w = dot(a.rows[3], b);

    return result;
}

inline f32 
clamp(f32 min, f32 value, f32 max)
{
    f32 result = value;
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
inline f32 
clamp01(f32 value)
{
    return clamp(0.0f, value, 1.0f);
}


inline u32
clamp(u32 min, u32 value, u32 max)
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
clamp(i32 min, i32 value, i32 max)
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

inline f32
lerp(f32 min, f32 t, f32 max)
{
    return min + t*(max-min);
}

inline v3
lerp(v3 min, f32 t, v3 max)
{
    v3 result = {};

    result.x = lerp(min.x, t, max.x);
    result.y = lerp(min.y, t, max.y);
    result.z = lerp(min.z, t, max.z);

    return result;
}

inline f32
max_element(v3 a)
{
    f32 result = maximum(maximum(a.x, a.y), a.z);

    return result;
}

inline f32
min_element(v3 a)
{
    f32 result = minimum(minimum(a.x, a.y), a.z);

    return result;
}

// 0x11 22 33 44
//    3  2  1  0 - little endian
//    0  1  2  3 - big endian
inline u32
big_to_little_endian(u32 big)
{
    u32 a = ((u8 *)&big)[0];
    u32 b = ((u8 *)&big)[1];
    u32 c = ((u8 *)&big)[2];
    u32 d = ((u8 *)&big)[3];

    // TODO(joon): should be a way to optimize this...
    u32 result = (((u8 *)&big)[0] << 24) | (((u8 *)&big)[1] << 16) | (((u8 *)&big)[2] << 8) | (((u8 *)&big)[3] << 0);

    return result;
}

inline u16
big_to_little_endian(u16 big)
{
    u16 result = (((u8 *)&big)[0] << 8) | (((u8 *)&big)[1] << 0);

    return result;
}

inline i16 
big_to_little_endian(i16 big)
{
    i16 result = (((i8 *)&big)[0] << 8) | (((i8 *)&big)[1] << 0);

    return result;
}

inline i32 
big_to_little_endian(i32 big)
{
    i32 result = (((i8 *)&big)[0] << 8) | (((i8 *)&big)[1] << 0);

    return result;
}

inline u64
big_to_little_endian(u64 byte_count)
{
    u64 result = 0;
    return result;
}

inline quat
Quat(f32 s, v3 v)
{
    quat result = {};
    result.s = s;
    result.v = v;

    return result;
}

// NOTE(joon) returns rotation quaternion
inline quat
Quat(v3 axis, f32 rad)
{
    quat result = {};
    result.s = cosf(rad);
    result.v = sinf(rad) * axis;

    return result;
}

inline quat
operator +(quat a, quat b)
{
    quat result = {};

    result.s = a.s + b.s;
    result.v = a.v + b.v;

    return result;
}

inline quat
operator *(quat a, quat b)
{
    quat result = {};

    result.s = a.s*b.s - dot(a.v, b.v);
    result.v = a.s*b.v + b.s*a.v + cross(a.v, b.v);

    return result;
}

inline quat
operator *(f32 value, quat q)
{
    quat result = q;
    result.s *= value;
    result.v *= value;

    return result;
}

inline quat
operator /(quat q, f32 value)
{
    quat result = {};
    result.s = q.s/value;
    result.v = q.v/value;

    return result;
}

inline quat &
operator *=(quat &q, f32 value)
{
    q.s *= value;
    q.v *= value;

    return q;
}

inline quat &
operator /=(quat &q, f32 value)
{
    q.s /= value;
    q.v /= value;

    return q;
}

// TODO(joon) make quaternion based on the orientation

inline f32
length_square(quat q)
{
    f32 result = length_square(q.v) + q.s*q.s;

    return result;
}

inline f32
length(quat q)
{
    f32 result = sqrt(length_square(q));

    return result;
}

inline quat
normalize(quat q)
{
    quat result = q/length(q);

    return result;
}

inline quat
conjugate(quat q)
{
    quat result = q;
    result.v *= -1.0f;

    return result;
}

inline quat
inverse(quat q)
{
    quat result = conjugate(q) / length_square(q);

    return result;
}

inline b32
does_quat_represent_orientation(quat q)
{
    b32 result = compare_with_epsilon(q.s, 0.0f) && compare_with_epsilon(length_square(q.v), 1.0f);

    return result;
}

// NOTE(joon) This matrix is an orthogonal matrix, so inverse == transpose
inline m3x3
rotation_quat_to_m3x3(quat q)
{
    m3x3 result = {};
    result.e[0][0] = 1 - 2.0f*(q.y*q.y + q.z*q.z);
    result.e[0][1] = 2.0f*(q.x*q.y + q.z*q.s);
    result.e[0][2] = 2.0f*(q.x*q.z - q.y*q.s);

    result.e[1][0] = 2.0f*(q.x*q.y - q.z*q.s);
    result.e[1][1] = 1 - 2.0f*(q.x*q.x + q.z*q.z);
    result.e[1][2] = 2.0f*(q.y*q.z - q.x*q.s);

    result.e[2][0] = 2.0f*(q.x*q.z - q.y*q.s);
    result.e[2][1] = 2.0f*(q.y*q.z - q.x*q.s);
    result.e[2][2] = 1 - 2.0f*(q.x*q.x + q.y*q.y);

    return result;
}

// NOTE(joon) 
// You can see that q before and after the rotation should be pure, as it is the only way
// they can exist in ijk coordinate.
// If the rotation axis is not perpendicular to 
// pure quaternion that we are trying to rotate,
// multiplying those two quaternions will make the q unpure.
// so we basically need to multiply it by inverse rot_q, 
// which makes q pure again. This is also why we are only using half_rad instead of rad
inline quat
rotate(quat q, v3 axis, f32 rad)
{
    assert(compare_with_epsilon(length_square(axis), 1.0f));
    assert(compare_with_epsilon(q.s, 0.0f));

    quat half_rot = Quat(axis, rad/2.0f);

    // Simplifed version of q * p * inverse(q)
    quat result = Quat(0, 2*dot(half_rot.v, q.v)*half_rot.v + (2*square(half_rot.s) - 1)*q.v +
                          2*half_rot.s*cross(half_rot.v, q.v));

    return result;
}


#endif

