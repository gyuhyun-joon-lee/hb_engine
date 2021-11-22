/*
 * Written by Gyuhyun 'Joon' Lee
 * https://github.com/meka-lopo/
 */
#ifndef MEKA_PLATFORM_H
#define MEKA_PLATFORM_H

#include <stdint.h>
#include <limits.h>
#include <float.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef int32_t b32;

typedef uint8_t u8; 
typedef uint16_t u16; 
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t uintptr;

#define U8_MAX UCHAR_MAX
#define U16_MAX USHRT_MAX
#define U32_MAX UINT_MAX

#define I8_MIN SCHAR_MIN
#define I8_MAX SCHAR_MAX
#define I16_MIN SHRT_MIN
#define I16_MAX SHRT_MAX
#define I32_MIN INT_MIN
#define I32_MAX INT_MAX

typedef float r32;
typedef double r64;

#define Assert(Expression) if(!(Expression)) {int *a = 0; *a = 0;}
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#define global_variable static
#define local_persist static
#define internal static

#define Kilobytes(value) value*1024LL
#define Megabytes(value) 1024LL*Kilobytes(value)
#define Gigabytes(value) 1024LL*Megabytes(value)
#define Terabytes(value) 1024LL*Gigabytes(value)

#define maximum(a, b) ((a>b)? a:b) 
#define minimum(a, b) ((a<b)? a:b) 

#include "math.h"
struct v2
{
    r32 x;
    r32 y;
};

inline v2
V2(r32 x, r32 y)
{
    v2 result = {};

    result.x = x;
    result.y = y;

    return result;
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
LengthSquare(v2 a)
{
    return a.x*a.x + a.y*a.y;
}

inline r32
Length(v2 a)
{
    return sqrtf(a.x*a.x + a.y*a.y);
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


struct v3
{
    r32 x;
    r32 y;
    r32 z;
};

inline v3
V3(r32 x, r32 y, r32 z)
{
    v3 result = {};

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
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
operator*(r32 value, v3 &a)
{
    v3 result = {};

    result.x = value*a.x;
    result.y = value*a.y;
    result.z = value*a.z;

    return result;
}

// NOTE(joon) : This assumes the vectors ordered counter clockwisely
inline v3
Cross(v3 a, v3 b)
{
    v3 result = {};

    result.x = a.y*b.z - b.y*a.z;
    result.y = -(a.x*b.z - b.x*a.z);
    result.z = a.x*b.y - b.x*a.y;

    return result;
}

inline r32
LengthSquare(v3 a)
{
    return a.x*a.x + a.y*a.y + a.z*a.z;
}

#include "math.h"
inline r32
Length(v3 a)
{
    return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
}

inline v3
Normalize(v3 a)
{
    return a/Length(a);
}


struct v4
{
    union
    {
        struct 
        {
            r32 x, y, z, w;
        };

        struct 
        {
            r32 r, g, b, a;
        };
        struct 
        {
            v3 xyz; 
            r32 ignored;
        };
        struct 
        {
            v3 rgb; 
            r32 ignored;
        };

        r32 e[4];
    };
};

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

inline r32
LengthSquare(v4 a)
{
    return a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w;
}

inline r32
Length(v4 a)
{
    return sqrtf(LengthSquare(a));
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

inline v4
Normalize(v4 a)
{
    return a/Length(a);
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

struct m4
{
    union
    {
        struct 
        {
            v4 column[4];
        };
        
        r32 e[16];
    };
};

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
operator*(m4 &a, m4 &b)
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
operator*(m4 &m, v4 v)
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
Scale(r32 x, r32 y, r32 z)
{
    m4 result = M4();

    result.column[0].e[0] *= x;
    result.column[1].e[1] *= y;
    result.column[2].e[2] *= z;

    return result;
}

inline m4
Translate(r32 x, r32 y, r32 z)
{
    m4 result = M4();

    result.column[3].e[0] = x;
    result.column[3].e[1] = y;
    result.column[3].e[2] = z;

    return result;
}

inline m4
View(v3 p, v3 lookatDir, v3 up)
{

    // TODO(joon) : No assert, but normalize here?
    lookatDir = -1.0f*Normalize(lookatDir);

    v3 right = Normalize(Cross(up, lookatDir));

    v3 newUp = Cross(lookatDir, right);

    m4 rotation = {};
    rotation.column[0] = {right.x, up.x, lookatDir.x, 0};
    rotation.column[1] = {right.y, up.y, lookatDir.y, 0};
    rotation.column[2] = {right.z, up.z, lookatDir.z, 0};
    rotation.column[3] = {0, 0, 0, 1};

    m4 translation = Translate(-p.x, -p.y, -p.z);

    return rotation*translation;
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

    result.column[0] = {m00, m10, m20, 0};
    result.column[1] = {m01, m11, m21, 0};
    result.column[2] = {m02, m12, m22, 0};
    result.column[3] = {0, 0, 0, 1};

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

// r = frustumHalfWidth
// t = frustumHalfHeight
// n = near plane z value
// f = far plane z value
inline m4
SymmetricProjection(r32 r, r32 t, r32 n, r32 f)
{
    m4 result = {};

    // IMPORTANT : In opengl, t corresponds to 1 -> column[1][1] = n/t
    // while in vulkan, t corresponds to -1 -> columm[1][1] = -n/t
    result.column[0] = V4(n/r, 0, 0, 0);
    result.column[1] = V4(0, -(n/t), 0, 0);
    result.column[2] = V4(0, 0, (n+f)/(n-f), -1);
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

inline void
Swap(u16 *a, u16 *b)
{
    u16 temp = *a;
    *a = *b;
    *b = temp;
}

struct platform_read_file_result
{
    u8 *memory;
    u32 size;
};
#define PLATFORM_READ_FILE(name) platform_read_file_result (name)(char *filename)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_FREE_FILE_MEMORY(name) void (name)(void *memory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

struct platform_api
{
    platform_read_file *ReadFile;
    platform_free_file_memory *FreeFileMemory;
};

internal u32
GetCharCountNullTerminated(const char *buffer)
{
    u32 result = 0;
    if(buffer)
    {
        while(buffer[result] != '\0')
        {
            ++result;
        }
    }

    return result;
}

// TODO(joon) : inefficient for long char arrays
// NOTE(joon) : No bound checking for the source char, which can be very dangerous!
internal b32
StringCompare(char *src, char *test)
{
    Assert(src && test);

    b32 result = true;
    u32 testCount = GetCharCountNullTerminated(test);

    for(u32 charIndex = 0;
        charIndex < testCount;
        ++charIndex)
    {
        if(src[charIndex] == '\0' ||
            src[charIndex] != test[charIndex])
        {
            result = false;
            break;
        }
    }

    return result;
}

struct platform_memory
{
    void *base;
    size_t size;
    size_t used;

    u32 tempMemoryStartCount;
    u32 tempMemoryEndCount;
};

struct temp_memory
{
    platform_memory *platformMemory;

    void *base;
    size_t size;
    size_t used;
};

internal temp_memory
StartTempMemory(platform_memory *platformMemory, size_t size)
{
    Assert(platformMemory->tempMemoryStartCount == platformMemory->tempMemoryEndCount || 
            platformMemory->tempMemoryEndCount == 0);

    temp_memory result = {};
    result.base = (u8 *)platformMemory->base + platformMemory->used;
    result.size = size;
    result.platformMemory = platformMemory;

    platformMemory->used += size;
    platformMemory->tempMemoryStartCount++;

    return result;
}
internal void
EndTempMemory(temp_memory *tempMemory)
{
    platform_memory *platformMemory = tempMemory->platformMemory;
    // NOTE(joon) : safe guard for using this temp memory after ending it 
    tempMemory->base = 0;
    platformMemory->tempMemoryEndCount++;
    // IMPORTANT(joon) : As the nature of this, all temp memories should be cleared at once
    platformMemory->used -= tempMemory->size;

    if(platformMemory->tempMemoryStartCount == platformMemory->tempMemoryEndCount)
    {
        platformMemory->tempMemoryStartCount = 0;
        platformMemory->tempMemoryEndCount = 0;
    }
}

// NOTE(joon): Works for both platform memory(world arena) & temp memory
#define PushArray(platformMemory, type, count) (type *)PushSize(platformMemory, count * sizeof(type))
#define PushStruct(platformMemory, type) (type *)PushSize(platformMemory, sizeof(type))

// TODO(joon) : Alignment might be an issue, always take account of that
internal void *
PushSize(platform_memory *platformMemory, size_t size, size_t alignment = 0)
{
    void *result = (u8 *)platformMemory->base + platformMemory->used;
    platformMemory->used += size;

    return result;
}

// TODO(joon) : Alignment might be an issue, always take account of that
internal void *
PushSize(temp_memory *tempMemory, size_t size, size_t alignment = 0)
{
    void *result = (u8 *)tempMemory->base + tempMemory->used;
    tempMemory->used += size;

    return result;
}




struct thread_work_queue;
#define THREAD_WORK_CALLBACK(name) void name(void *data)
typedef THREAD_WORK_CALLBACK(thread_work_callback);

struct thread_work_item
{
    thread_work_callback *callback;
    void *data;

    b32 written; // indicates whether this item is properly filled or not
};

#define PLATFORM_FINISH_ALL_THREAD_WORK_QUEUE_ITEMS(name) void name(thread_work_queue *queue)
typedef PLATFORM_FINISH_ALL_THREAD_WORK_QUEUE_ITEMS(platform_finish_all_thread_work_queue_items);

#define PLATFORM_ADD_THREAD_WORK_QUEUE_ITEM(name) void name(thread_work_queue *queue, thread_work_callback *threadWorkCallback, void *data)
typedef PLATFORM_ADD_THREAD_WORK_QUEUE_ITEM(platform_add_thread_work_queue_item);

// IMPORTANT(joon): There is no safeguard for the situation where one work takes too long, and meanwhile the work queue was filled so quickly
// causing writeItem == readItem
struct thread_work_queue
{
    // NOTE(joon) : volatile forces the compiler not to optimize the value out, and always to the load(as other thread can change it)
    u32 volatile writeIndex;
    u32 volatile readIndex;

    thread_work_item items[256];

    // now this can be passed onto other codes, such as seperate game code to be used as rendering 
    platform_add_thread_work_queue_item *AddThreadWorkQueueItem;
    platform_finish_all_thread_work_queue_items * FinishAllThreadWorkQueueItems;
};

#endif
