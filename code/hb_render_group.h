#ifndef HB_RENDER_GROUP_H
#define HB_RENDER_GROUP_H

// TODO(gh) To seperate the 'asset' thing completely from the game code
// whenever we need some kind of mesh, retrieve from the platform layer
// that already has the handle to the vertexbuffer, texture... etc?

// NOTE(gh) : This forces us to copy these datas again to the actual 'usable' vertex buffer,
// but even with more than 100000 vertices, the size of it isn't too big 

struct RawMesh
{
    v3 *positions;
    u32 position_count;

    v3 *normals;
    u32 normal_count;

    v2 *texcoords;
    u32 texcoord_count;

    u32 *indices;
    u32 index_count;

    u32 *normal_indices;
    u32 normal_index_count;

    u32 *texcoord_indices;
    u32 texcoord_index_count;
};

/* 
   TODO(gh) we would want these functionalities for the camera
   - Initialize camera with the 'position' and the 'look at' position.
   - fully functioning quaternion to avoid gimbal lock
*/

/*
   NOTE(gh) To avoid any confusion, the following things are true
   - By default, camera coordinates = world coordinates
   - camera -Z is the lookat direction
*/

struct Camera
{
    f32 pitch; // x
    f32 yaw; // y
    f32 roll; // z

    // TODO(gh) someday, tricky part is not the rotation itself, but 
    // initializing but the camera so that it can look at some position
    quat orientation;

    v3 p;
    f32 focal_length;
};

enum RenderEntryType
{
    RenderEntryType_AABB,
    RenderEntryType_Line,
    RenderEntryType_Cube,
    RenderEntryType_Sphere,
};

// TODO(gh) Do we have enough reason to keep this header?
struct RenderEntryHeader
{
    // NOTE(gh) This should _always_ come first
    RenderEntryType type;
};

struct RenderEntryAABB
{
    RenderEntryHeader header;

    v3 p;
    v3 dim;

    v3 color;
};

struct RenderEntryLine
{
    RenderEntryHeader header;

    v3 start;
    v3 end;
    v3 color;
};

struct RenderEntryCube
{
    RenderEntryHeader header;

    v3 p;
    v3 dim;
    // TODO(gh) Should we store the full matrix here to reduce time from
    // getting the rendering material to acutally drawing it to the screen?(especially for Metal)
    quat orientation; // NOTE(gh) this should be a pure quaternion
    v3 color;
};

struct RenderEntrySphere
{
    RenderEntryHeader header;

    v3 p;
    v3 r;
    v3 color;
};

#if 0
struct RenderEntryParticleFaces
{
    RenderEntryHeader header;
    ParticleFaces *faces;
    u32 face_count;
};
#endif

// TODO(gh) Do we even need to keep the concept of 'render group'
struct RenderGroup
{
    // NOTE(gh) provided by the platform layer
    PlatformRenderPushBuffer *render_push_buffer;
};

// TODO(gh) move the shader related structs(i.e uniform) into seperate file? (hb_shader.h)
struct PerFrameData
{
    alignas(16) m4x4 proj_view;
};

struct PerObjectData
{
    alignas(16) m4x4 model;
    alignas(16) v3 color;
};

#endif
