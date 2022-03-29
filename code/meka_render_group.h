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

// TODO(joon) pack a bunch of host coherent & visible buffer, save the offset of each of them, and then
// pass to the GPU?
struct Uniform
{
    // TODO(joon) move to per object uniform buffer
    m4 model;
    m4 proj_view;

    alignas(16) v3 light_p;
};

/*
   NOTE(joon): Here's how we construct the camera
   Initial camera axis : 
    camera_x : (0, -1, 0)
    camera_y : (0, 0, 1)
    camera_z : (-1, 0, 0)

    lookat dir : (1, 0, 0)

    This gives us more reasonable world coordinate when we move from camera space to world space 
*/
// TODO(joon) make this to be entirely quarternion-based
struct Camera
{
    // TODO(joon): dont need to store these values, when we finalize the camera code?
    v3 initial_x_axis;
    v3 initial_y_axis;
    v3 initial_z_axis;

    v3 p;

    f32 focal_length;

    union
    {
        struct
        {
            r32 along_x;
            r32 along_y;
            r32 along_z;
        };

        struct
        {
            r32 roll; 
            r32 pitch; 
            r32 yaw;  
        };

        r32 e[3];
    };
};

enum RenderEntryType
{
    Render_Entry_Type_AABB,
    Render_Entry_Type_Line,
};

// TODO(joon) Do we have enough reason to keep this header?
struct RenderEntryHeader
{
    // NOTE(joon) This should _always_ come first
    RenderEntryType type;
};

struct RenderEntryVoxelChunk
{
    RenderEntryHeader header;

    void *chunk_data; // just storing the pointer
    u32 chunk_size; // around 0.2mb
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

// TODO(joon) Do we even need to keep the concept of 'render group'
struct RenderGroup
{
    // NOTE(joon) provided by the platform layer
    PlatformRenderPushBuffer *render_push_buffer;
};

#endif
