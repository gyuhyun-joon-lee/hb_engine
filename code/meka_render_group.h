#ifndef MEKA_RENDER_GROUP_H
#define MEKA_RENDER_GROUP_H

// NOTE(joon) : This forces us to copy these datas again to the actual 'usable' vertex buffer,
// but even with more than 100000 vertices, the size of it isn't too big 
struct raw_mesh
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

struct Camera
{
    f32 pitch;
    f32 yaw;
    f32 roll;

    // TODO(joon) someday...
    // TODO(joon) Will getting the axis_angle representation from pitch, yaw, and roll
    // and using that to rotate the orientation work?
    //quat orientation;

    v3 p;
    f32 focal_length;
};

enum RenderEntryType
{
    RenderEntryType_AABB,
    RenderEntryType_Line,
    RenderEntryType_Cube,
};

// TODO(joon) Do we have enough reason to keep this header?
struct RenderEntryHeader
{
    // NOTE(joon) This should _always_ come first
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
    // TODO(joon) Should we store the full matrix here to reduce time from
    // getting the rendering material to acutally drawing it to the screen?(especially for Metal)
    quat orientation; // NOTE(joon) this should be a pure quaternion
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

// TODO(joon) Do we even need to keep the concept of 'render group'
struct RenderGroup
{
    // NOTE(joon) provided by the platform layer
    PlatformRenderPushBuffer *render_push_buffer;
};

// TODO(joon) move the shader related structs(i.e uniform) into seperate file? (meka_shader.h)
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
