/*
 * Written by Gyuhyun Lee
 */

#ifndef HB_RENDER_GROUP_H
#define HB_RENDER_GROUP_H

/*
   Later down the road, we might wanna thing about this, if there are a lot of things to render.
*/

struct CommonVertex
{
    v3 p;
    v3 normal;
};

struct GrassGrid
{
    b32 should_draw;

    u32 grass_count_x;
    u32 grass_count_y;

    // NOTE(gh) Platform layer should give enough memory to each of the buffer to 
    // hold grass_count_x*grass_count_y amount of elements. 
    u32 *hash_buffer;
    u32 hash_buffer_size;
    u32 hash_buffer_offset; // offset to the giant buffer, game code should not care about this
    b32 updated_hash_buffer;

    f32 *floor_z_buffer;
    u32 floor_z_buffer_size;
    u32 floor_z_buffer_offset; // offset to the giant buffer, game code should not care about this
    b32 updated_floor_z_buffer;

    // TODO(gh) Also store offset x and y for smooth transition between grids, 
    // or maybe make perlin noise to be seperate from the grid?
    // For now, it is given free because of the fact that every grid is 256 x 256
    f32 *perlin_noise_buffer;
    u32 perlin_noise_buffer_size;
    u32 perlin_noise_buffer_offset; // offset to the giant buffer, game code should not care about this

    v2 min;
    v2 max;
};

struct CameraFrustum
{
    // morten z order
    v3 near[4];
    v3 far[4];
};

/* 
   TODO(gh) we would want these functionalities for the camera
   - Initialize camera with the 'position' and the 'look at' position.
   - fully functioning quaternion to avoid gimbal lock

   As you mention the gimbal lock issue arises anytime you do three consecutive rotations (such as Euler Angles) 
   to get from an inertial coordinate frame to a body frame. 
   This includes combining three successive quaternion rotations (through a operation called composition).

    The reason why quaternions can overcome gimbal lock is that they can represent the transformation from the inertial coordinate frame to the body fixed frame in a single rotation. 
    This is however imho the big disadvantage of quaternions - it is not physically intuitive to come up with a desired quaternion.
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

#if 0
quat lookAt(v3 source, v3 dest, v3 front, v3 up)
{
    v3 toVector = normalize(dest - source);

    //compute rotation axis
    Vector3f rotAxis = front.cross(toVector).normalized();
    if (rotAxis.squaredNorm() == 0)
        rotAxis = up;

    //find the angle around rotation axis
    float dot = VectorMath::front().dot(toVector);
    float ang = std::acosf(dot);

    //convert axis angle to quaternion
    return Eigen::AngleAxisf(rotAxis, ang);
}

//Angle-Axis to Quaternion
Quaternionr angleAxisf(const Vector3r& axis, float angle) {
    auto s = std::sinf(angle / 2);
    auto u = axis.normalized();
    return Quaternionr(std::cosf(angle / 2), u.x() * s, u.y() * s, u.z() * s);
}

#endif

struct Camera
{
    // TODO(gh) This should work fine for now, as long as we don't use all three axes for rotation
    // which causes gimbal lock in euler rotation.
    f32 pitch; // x
    f32 yaw; // y
    f32 roll; // z

    // TODO(gh) Someday! Tricky part is not the rotation itself, but 
    // initializing the camera so that it can look at some position
    quat orientation;

    f32 near;
    f32 far;
    f32 fov;

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
    RenderEntryType_Cube,
    RenderEntryType_Grass,
    RenderEntryType_Frustum,
};

// TODO(gh) Do we have enough reason to keep this header?
struct RenderEntryHeader
{
    // NOTE(gh) This should _always_ come first
    RenderEntryType type;
    // We store this to automatically advance inside the render entry buffer
    // This can be much smaller than 4 bytes to save space
    u32 size;
};

struct RenderEntryAABB
{
    RenderEntryHeader header;

    v3 p;
    v3 dim;
    v3 color;

    b32 should_cast_shadow;

    // TODO(gh) Make this as a struct?
    // NOTE(gh) offset to the combined vertex & index buffer
    u32 vertex_buffer_offset;
    u32 index_buffer_offset;
    u32 index_count;
};

struct RenderEntryCube
{
    RenderEntryHeader header;

    v3 p;
    v3 dim;
    v3 color;

    b32 should_cast_shadow;

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

    b32 should_cast_shadow;

    // NOTE(gh) offset to the combined vertex & index buffer
    u32 vertex_buffer_offset;
    u32 index_buffer_offset;
    u32 index_count;
};

struct RenderEntryFrustum
{
    RenderEntryHeader header;
    v3 color;

    // NOTE(gh) offset to the combined vertex & index buffer
    u32 vertex_buffer_offset;
    u32 index_buffer_offset;
    u32 index_count;
};



#endif
