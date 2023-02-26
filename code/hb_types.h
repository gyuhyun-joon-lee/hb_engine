/*
 * Written by Gyuhyun Lee
 */

#ifndef HB_TYPES_H
#define HB_TYPES_H

#include <stdint.h>
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

typedef float f32;
typedef float f32;
typedef double f64;

#define Flt_Min FLT_MIN
#define Flt_Max FLT_MAX

#define flt_min FLT_MIN
#define flt_max FLT_MAX

#define U8_Max UINT8_MAX
#define U16_Max UINT16_MAX
#define U32_Max UINT32_MAX

#define I32_Min INT32_MIN
#define I32_Max INT32_MAX
#define I16_Min INT16_MIN
#define I16_Max INT16_MAX
#define I8_Min INT8_MIN
#define I8_Max INT8_MAX

#define u8_max UINT8_MAX
#define u16_max UINT16_MAX
#define u32_max UINT32_MAX

#define i32_min INT32_MIN
#define i32_max INT32_MAX
#define i16_min INT16_MIN
#define i16_max INT16_MAX
#define i8_min INT8_MIN
#define i8_max INT8_MAX

#define assert(expression) if(!(expression)) {int *adfasdfasdfasdf = 0; *adfasdfasdfasdf = 0;}
#define array_count(array) (sizeof(array) / sizeof(array[0]))
#define array_size(array) (sizeof(array))
#define invalid_code_path assert(0)

#define global static
#define global_variable static
#define local_persist static
#define internal static

#define kilobytes(value) value*1024LL
#define megabytes(value) 1024LL*kilobytes(value)
#define gigabytes(value) 1024LL*megabytes(value)
#define terabytes(value) 1024LL*gigabytes(value)

#define sec_to_nanosec 1.0e+9f
#define sec_to_millisec 1000.0f
//#define nano_sec_to_micro_sec 0.0001f // TODO(gh): Find the correct value :(

#define maximum(a, b) ((a>b)? a:b) 
#define minimum(a, b) ((a<b)? a:b) 

// NOTE(gh): *(u32 *)c == "stri" does not work because of the endianess issues
#define four_cc(string) (((string[0] & 0xff) << 0) | ((string[1] & 0xff) << 8) | ((string[2] & 0xff) << 16) | ((string[3] & 0xff) << 24))

#define tau_32 6.283185307179586476925286766559005768394338798750211641949889f

#define pi_32 3.14159265358979323846264338327950288419716939937510582097494459230f
#define half_pi_32 (pi_32/2.0f)
#define euler_contant 2.7182818284590452353602874713526624977572470936999595749f
#define degree_to_radian(degree) ((degree / 180.0f)*pi_32)

// NOTE(joon) Structs that are in this file start with lower case
// as they are easier to type imo.

typedef struct v2
{
    f32 x;
    f32 y;
}v2;

typedef struct v2d
{
    f64 x;
    f64 y;
}v2d;

typedef struct v2u
{
    u32 x;
    u32 y;
}v2u;

typedef struct v3
{
    union
    {
        struct 
        {
            f32 x;
            f32 y;
            f32 z;
        };
        struct 
        {
            f32 r;
            f32 g;
            f32 b;
        };
        struct 
        {
            v2 xy;
            f32 ignored;
        };

        f32 e[3];
    };
}v3;

typedef struct v3d
{
    union
    {
        struct 
        {
            f64 x;
            f64 y;
            f64 z;
        };
        struct 
        {
            f64 r;
            f64 g;
            f64 b;
        };
        struct 
        {
            v2d xy;
            f64 ignored;
        };

        f64 e[3];
    };
}v3d;

struct v3u
{
    u32 x;
    u32 y;
    u32 z;
};

struct v3i
{
    i32 x;
    i32 y;
    i32 z;
};

struct v4
{
    union
    {
        struct 
        {
            f32 x, y, z, w;
        };

        struct 
        {
            f32 r, g, b, a;
        };
        struct 
        {
            v3 xyz; 
            f32 ignored;
        };
        struct 
        {
            v3 rgb; 
            f32 ignored1;
        };

        f32 e[4];
    };
};

struct v9
{
    union
    {
        struct
        {
            f32 e0, e1, e2, e3, e4, e5, e6, e7, e8;
        };

        f32 e[9];
    };
};

struct v9d
{
    union
    {
        struct
        {
            f64 e0, e1, e2, e3, e4, e5, e6, e7, e8;
        };

        f64 e[9];
    };
};

// NOTE(joon) quat is RHS
struct quat
{
    union
    {
        struct
        {
            // NOTE(joon) ordered pair representation of the quaternion
            // q = [s, v] = S + Vx*i + Vy*j + Vz*k
            f32 s; // scalar
            v3 v; // vector
        };

        struct 
        {
            f32 w;
            f32 x;
            f32 y;
            f32 z;
        };
    };
};

struct quatd
{
    union
    {
        struct
        {
            // NOTE(joon) ordered pair representation of the quaternion
            // q = [s, v] = S + Vx*i + Vy*j + Vz*k
            f64 s; // scalar
            v3d v; // vector
        };

        struct 
        {
            f64 w;
            f64 x;
            f64 y;
            f64 z;
        };
    };
};

// TODO(gh) For these matrices, we should check whether 
// the compiler is smart enough to inline the math functions,
// since the matrix can be huge m3x3d = 8 * 3 * 3 = 72 bytes!!! (or maybe just use &...)

// row major
// e[0][0] e[0][1] 
// e[1][0] e[1][1]
// e[2][0] e[2][1]
struct m2x2
{
    union
    {
        struct
        {
            v2 rows[2];
        };

        // [row][column]
        f32 e[2][2];
    };
};

// row major
// e[0][0] e[0][1] e[0][2]
// e[1][0] e[1][1] e[1][2]
// e[2][0] e[2][1] e[2][2]
struct m3x3
{
    union
    {
        struct
        {
            v3 rows[3];
        };

        // [row][column]
        f32 e[3][3];
    };
};

// row major
// e[0][0] e[0][1] e[0][2]
// e[1][0] e[1][1] e[1][2]
// e[2][0] e[2][1] e[2][2]
struct m3x3d
{
    union
    {
        struct
        {
            v3d rows[3];
        };

        // [row][column]
        f64 e[3][3];
    };
};

// row major
// e[0][0] e[0][1] e[0][2] e[0][3]
// e[1][0] e[1][1] e[1][2] e[1][3]
// e[2][0] e[2][1] e[2][2] e[2][3]
// NOTE(gh) This matrix is mostly used for transformation which doesn't require homogeneous coords,
// such as camera transform or rigid body transform
struct m3x4
{
    union
    {
        struct 
        {
            v4 rows[3];
        };

        // [row][column]
        f32 e[3][4];
    };
};

// NOTE(joon) row major
// e[0][0] e[0][1] e[0][2] e[0][3]
// e[1][0] e[1][1] e[1][2] e[1][3]
// e[2][0] e[2][1] e[2][2] e[2][3]
// e[3][0] e[3][1] e[3][2] e[3][3]
struct m4x4
{
    union
    {
        struct 
        {
            v4 rows[4];
        };

        // [row][column]
        f32 e[4][4];
    };
};

// NOTE(gh) This matrix is hardly ever used,
// except when getting the shape matching matrix
struct m9x9d
{
    union
    {
        struct 
        {
            v9d rows[9];
        };

        // [row][column]
        f64 e[9][9];
    };
};


#endif


