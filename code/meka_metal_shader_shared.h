// NOTE(joon) : This file is directly shared with the metal shader
#ifndef MEKA_METAL_H
#define MEKA_METAL_H

// NOTE(joon): According to Ilgwon Ha, these vectors have 16 bytes alignment
typedef float r32_2 __attribute__((ext_vector_type(2)));
typedef float r32_3 __attribute__((ext_vector_type(3)));
typedef float r32_4 __attribute__((ext_vector_type(4)));
typedef float r32_8 __attribute__((ext_vector_type(8)));
typedef float r32_16 __attribute__((ext_vector_type(16)));

// TODO(joon) : Any need to have mulitiple types of vertices,
// i.e if the vertex is very small, does it increae the vertex cache hit chance?
typedef struct
{
    alignas(16) r32_3 p;
    alignas(16) r32_3 normal;
}temp_vertex;

typedef struct
{
    r32_4 column[4];
}r32_4x4;

typedef struct
{
    alignas(16) r32_4x4 proj_view;
    alignas(16) r32_3 light_p;
}per_frame_data;

typedef struct
{
    alignas(16) r32_4x4 model;
}per_object_data;


inline r32_4
convert_to_r32_4(r32_3 v, float w)
{ 
    r32_4 result = {};

    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    result.w = w;

    return result;
}

inline r32_3
convert_to_r32_3(r32_4 v)
{ 
    r32_3 result = {};

    result.x = v.x;
    result.y = v.y;
    result.z = v.z;

    return result;
}

inline r32_4
operator*(r32_4x4 m, r32_4 v)
{
    r32_4 result = {};
    result.x = m.column[0].x*v.x + 
            m.column[1].x*v.y +
            m.column[2].x*v.z +
            m.column[3].x*v.w;

    result.y = m.column[0].y*v.x + 
            m.column[1].y*v.y +
            m.column[2].y*v.z +
            m.column[3].y*v.w;

    result.z = m.column[0].z*v.x + 
            m.column[1].z*v.y +
            m.column[2].z*v.z +
            m.column[3].z*v.w;

    result.w = m.column[0].w*v.x + 
            m.column[1].w*v.y +
            m.column[2].w*v.z +
            m.column[3].w*v.w;

    return result;
}

#endif
