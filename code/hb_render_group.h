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
    - By default, camera coordinates = world coordinates (if the camera is not a circling camera)
    - camera -Z is the lookat direction. View matrix will take account of this, 
      and transform the world coordinate into the camera coordinate.
    - focal length == near plane value(in world unit).
      However, because the camera -Z is the lookat direction, it will be negated before being used
      in the projection matrix.
    - So before the projection matrix, near < far (i.e 1 < 5). 
      However, in the projection matrix, n > f (i.e -1 > -5) because both values will be negated.
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

// camera that circles around a single axis, looking at certain poitn
struct CircleCamera
{
    v3 p;
    v3 lookat_p;

    f32 distance_from_axis;
    f32 rad;
    // TODO(gh) Allow this camera to rotate along any axis!

    // NOTE(gh) values along the camera z axis
    f32 near;
    f32 far;

    f32 fov; // in radian
};

enum RenderEntryType
{
    RenderEntryType_AABB,
    RenderEntryType_Line,
    RenderEntryType_Grass,
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

    // TODO(gh) Make this as a struct?
    // NOTE(gh) offset to the combined vertex & index buffer
    u32 vertex_buffer_offset;
    u32 index_buffer_offset;
    u32 index_count;
};

struct RenderEntryLine
{
    RenderEntryHeader header;

    v3 start;
    v3 end;
    v3 color;
};

struct RenderEntryGrass
{
    RenderEntryHeader header;

    v3 p;
    v3 dim;
    v3 color;

    // NOTE(gh) offset to the combined vertex & index buffer
    u32 vertex_buffer_offset;
    u32 index_buffer_offset;
    u32 index_count;
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
