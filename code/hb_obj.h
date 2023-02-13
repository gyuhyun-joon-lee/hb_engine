#ifndef PARSER_H
#define PARSER_H

struct ParseNumericResult
{
    b32 is_float;
    union
    {
        i32 value_i32;
        f32 value_f32;
    };
};

enum ObjTokenType
{
    obj_token_type_null,

    obj_token_type_v,
    obj_token_type_vn,
    obj_token_type_vt,
    obj_token_type_f,
    obj_token_type_i32,
    obj_token_type_f32,
    obj_token_type_slash,
    //obj_token_type_hyphen,
    obj_token_type_comment,
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

enum ObjVertexType
{
    obj_vertex_type_v,
    obj_vertex_type_v_vn,

    // TODO(joon) not supported
    obj_vertex_type_v_vt,
    obj_vertex_type_v_vt_vn,
};

struct PreParseObjResult
{
    u32 position_count;
    u32 normal_count;
    u32 texcoord_count;
    u32 index_count;

    ObjVertexType vertex_type;

    u32 property_count;
};

enum PlyTokenType
{
    ply_token_type_null, 
    ply_token_type_ply,  

    ply_token_type_format,
    ply_token_type_ascii,

    // skip to next newline
    ply_token_type_comment,
    
    ply_token_type_element,
    ply_token_type_vertex,

    ply_token_type_property,
    ply_token_type_confidence,
    ply_token_type_intensity,

    ply_token_type_face,
    ply_token_type_list,

    // NOTE(joon) either one have the same meaning, just differnet syntax
    ply_token_type_vertex_index,
    ply_token_type_vertex_indices,

    ply_token_type_end_header,

    // types that can be written inside the header
    ply_token_type_float,
    ply_token_type_float32,
    ply_token_type_uint8,
    ply_token_type_uchar,
    ply_token_type_uint16,
    ply_token_type_int16,
    ply_token_type_int,
    ply_token_type_int32,

    // values
    ply_token_type_f32,
    ply_token_type_i32,
};

struct PlyToken
{
    PlyTokenType type;
    b32 is_float;
    union
    {
        f32 value_f32;
        i32 value_i32;
    };
};

struct ParsePlyHeaderResult
{
    u32 vertex_count;
    u32 vertex_property_count;

    u32 index_count;
};

struct Tokenizer
{
    u8 *at;
    u8 *one_past_end;
};

#if 0
enum SceneTokenType
{
    scene_token_type_null, 

    scene_token_type_screen,
    scene_token_type_camera,
    scene_token_type_ambient,
    scene_token_type_light,
    scene_token_type_sphere,
    scene_token_type_brdf,
    scene_token_type_box,
    scene_token_type_cylinder,
    scene_token_type_mesh,

    scene_token_type_f32,
    scene_token_type_i32,
    //scene_token_type_hyphen, // TODO(joon) we can combine this to the numeric value parse routine
    scene_token_type_string,

    scene_token_type_b,
    scene_token_type_q,
    scene_token_type_z,
};

struct SceneToken
{
    SceneTokenType type;

    u8 *start;
    union
    {
        f32 value_f32;
        i32 value_i32;
    };
};

struct MeshInfo
{
    // TODO(joon) If we don't want any memory allocation inside the parser,
    // we can just get the file_path_start & file_path_end
    char *file_path;

    u32 mat_index;
};

struct ParseSceneResult
{
    i32 output_width;
    i32 output_height;

    v3 camera_p;
    f32 camera_height_ratio;
    v4 camera_quarternion;

    v3 ambient;

    // TODO(joon) pass world instead?
    Sphere spheres[100]; // TODO(joon) better way??
    u32 sphere_count;

    Box boxes[100];
    u32 box_count;

    Cylinder cylinders[100];
    u32 cylinder_count;

    Material materials[100];
    u32 mat_count;

    MeshInfo mesh_infos[100];
    u32 mesh_count;
};
#endif


#endif
