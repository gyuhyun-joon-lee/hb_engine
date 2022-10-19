#ifndef HB_OBJ_H
#define HB_OBJ_H

/*
    NOTE(gyuhyun) : 
    1. Peek token does not peek the string token and will assert if if finds anything that cannot be recognized. 
    Instead, the one who actually cares about the string token(which pretty much means the parser)
    will 'peek' the string token and advance the tokenizer.

    The more general way of doing this would be tokenizing everything(including the whitespace, newline, string ...)
    and use them, but we don't quite need this because the format for the obj and mtl files are already defined and 
    pretty simple.
*/

enum MtlTokenType
{
    MtlTokenType_null,

    MtlTokenType_newmtl,
    MtlTokenType_Kd,
    MtlTokenType_Ke,
    MtlTokenType_Ns,
    MtlTokenType_d,
    MtlTokenType_illum,
    MtlTokenType_Ka,
    MtlTokenType_Ks,
    MtlTokenType_Ni,
    MtlTokenType_map_Kd,

    MtlTokenType_i32,
    MtlTokenType_f32,
    MtlTokenType_hyphen,
};

struct MtlTokenizer
{
    u8 *at;
    u8 *end;

    MtlTokenType _previous_token_type;
};

struct MtlToken
{
    MtlTokenType type;

    union
    {
        i32 value_i32;
        f32 value_f32;
    };
};

struct MtlPreParseResult
{
    u32 total_mtl_count;
};

enum ObjTokenType
{
    ObjTokenType_null,

    ObjTokenType_v,
    ObjTokenType_vn,
    ObjTokenType_vt,
    ObjTokenType_f,
    ObjTokenType_i32,
    ObjTokenType_f32,
    ObjTokenType_slash,
    ObjTokenType_hyphen,
    ObjTokenType_o, // objects 
    ObjTokenType_newline, // used to flush vertex information, face information...

    // NOTE(gyuhyun) not supported or ignored
    ObjTokenType_s,
    ObjTokenType_off,
    ObjTokenType_mtllib,
    ObjTokenType_g,
    ObjTokenType_usemtl,
};

struct ObjTokenizer
{
    u8 *at;
    u8 *end;

    // NOTE(gyuhyun) storing previous token generates too much headache..(c++ is terrible at this)
    // let's just hope we never have to store the actual token XD
    // Also, the value means the previous token type only inside the peek token function.
    ObjTokenType _previous_token_type;
};

struct ObjToken
{
    ObjTokenType type;

    union
    {
        i32 value_i32;
        f32 value_f32;
    };
};

struct ObjStringToken
{
    char string[64];
    u32 string_size;
};

struct ObjMtl
{
    v3 Kd;
    v3 Ns;
    v3 d;
    v3 illum;
    v3 Ka;
    v3 Ks;

    char name[64];
};

struct ObjPreParseResult
{
    // NOTE(joon) One obj file might contain more than one 'object',
    // and this is the total amount inside one obj file(not the object!).
    u32 total_position_count;
    u32 total_normal_count;
    u32 total_texcoord_count;

    u32 total_position_index_count;
    u32 total_normal_index_count;
    u32 total_texcoord_index_count;

    u32 total_object_count;

    u32 total_mtl_info_count;
};

struct ObjMtlInfo
{
    u32 mtl_index; // index to the ObjMtl

    // index to the vertex index
    u32 first_index_index;
    u32 one_past_last_index_index;
};

// NOTE(gyuhyun) This only represents one object in the obj file
// Also, one object might use different mtls - 
// in this case we will treat this as different objects
struct LoadedObjObject
{
    char name[64];

    v3 *positions;
    v3 *normals;
    v2 *texcoords;

    u32 position_count;
    u32 normal_count;
    u32 texcoord_count;

    u32 *position_indices;
    u32 *normal_indices;
    u32 *texcoord_indices;

    u32 position_index_count;
    u32 normal_index_count;
    u32 texcoord_index_count;

    u32 mtl_info_count;
    ObjMtlInfo *mtl_infos;
};

// NOTE(gyuhyun) user needs to provide these pointers,
// and should be big enough to hold the values that are specified in objpre parse result
struct ObjParseMemory
{
    LoadedObjObject *loaded_objects; // should have at least object_count in preparse

    v3 *positions;
    v3 *normals;
    v2 *texcoords;

    u32 *position_indices;
    u32 *normal_indices;
    u32 *texcoord_indices;

    ObjMtlInfo *mtl_infos;
};























#endif
