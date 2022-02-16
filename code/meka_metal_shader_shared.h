// NOTE(joon) : This file is directly shared with the metal shader,
// and will always follow the alignment that the shader is requiring(i.e float3 has 16 bytes alignment)
#ifndef MEKA_SHARED_WITH_METAL_SHADER_H
#define MEKA_SHARED_WITH_METAL_SHADER_H

// NOTE(joon): According to Ilgwon Ha, these vectors have 16 bytes alignment
typedef float f32x2 __attribute__((ext_vector_type(2)));
typedef float f32x3 __attribute__((ext_vector_type(3)));
typedef float f32x4 __attribute__((ext_vector_type(4)));
typedef float f32x8 __attribute__((ext_vector_type(8)));
typedef float f32x16 __attribute__((ext_vector_type(16)));

#include "meka_types.h"

struct PerFrameData
{
    union
    {
        alignas(16) m4 proj_view_;
        f32x4 proj_view;
    };
};

#endif
