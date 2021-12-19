#ifndef MEKA_TYPES_H
#define MEKA_TYPES_H

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef int32_t b32;

typedef uint8_t u8; 
typedef uint16_t u16; 
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef float f32;
#ifndef MEKA_METAL_SHADER
typedef double r64;
#endif

#define flt_min FLT_MIN
#define flt_max FLT_MAX
#define u32_max UINT32_MAX
#define u16_max UINT16_MAX
#define u8_max UINT8_MAX

#define i32_min INT32_MIN
#define i32_max INT32_MAX
#define i16_min INT16_MIN
#define i16_max INT16_MAX
#define i8_min INT8_MIN
#define i8_max INT8_MAX

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




#endif
