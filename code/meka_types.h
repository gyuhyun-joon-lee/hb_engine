#ifndef MEKA_TYPES_H
#define MEKA_TYPES_H

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8; 
typedef uint16_t u16; 
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
#if! METAL_SHARED
typedef double r64;
#endif

struct v2
{
    r32 x;
    r32 y;
};

struct v3
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
            v2 xy;
            r32 ignored_0;
        };

        struct 
        {
            r32 r;
            r32 g;
            r32 b;
        };
        struct 
        {
            r32 ignored_1;
            v2 yz;
        };

        r32 e[4];
    };
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
            r32 ignored_0;
        };
        struct 
        {
            v3 rgb; 
            r32 ignored_1;
        };

        r32 e[4];
    };
};

#endif
