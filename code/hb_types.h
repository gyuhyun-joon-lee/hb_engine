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

typedef float r32;
typedef float f32;
typedef double r64;

#define Flt_Min FLT_MIN
#define Flt_Max FLT_MAX

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

// NOTE(joon) As you may have noticed, structs that are in this file start with lower case,
// compared to other structs in the codebase.

typedef struct v2
{
    r32 x;
    r32 y;
}v2;

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
            r32 x;
            r32 y;
            r32 z;
        };
        struct 
        {
            r32 r;
            r32 g;
            r32 b;
        };
        struct 
        {
            v2 xy;
            r32 ignored;
        };

        r32 e[3];
    };
}v3;

struct v3u
{
    u32 x;
    u32 y;
    u32 z;
};

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
            r32 ignored1;
        };

        r32 e[4];
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

struct m4
{
    union
    {
        struct 
        {
            v4 column[4];
        };
        
        f32 e[16];
    };
};

// row major
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

#endif
