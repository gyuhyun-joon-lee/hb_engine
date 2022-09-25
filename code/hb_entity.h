#ifndef HB_ENTITY_H
#define HB_ENTITY_H

enum EntityType
{
    EntityType_Null,
    EntityType_AABB,
    EntityType_Floor,
    EntityType_Cube,
    EntityType_Grass,
};

enum EntityFlag
{
    Entity_Flag_Null = 0,
    Entity_Flag_Movable = 2,
    Entity_Flag_Collides = 4,
};

struct Entity
{
    u32 ID;
    EntityType type;
    u32 flags;

    v3 p; // TODO(gh) such objects like rigid body does not make use of this!!!
    v3 dim;
    v3 color;

    b32 should_cast_shadow;

    // NOTE(gh) bezier curve properties
    // TODO(gh) us dim for these values?
    f32 blade_width;
    f32 stride;
    f32 height;

    u32 grass_divided_count;
    f32 tilt; // == p2.z
    f32 bend; // controls p1
    v2 tilt_direction; // only x and y values, z == 0
    u32 hash;

    CommonVertex *vertices;
    u32 vertex_count;

    // TODO(gh) We don't really need to hold the indices, as long as we have the index buffer.
    u32 *indices; 
    u32 index_count;

    // TODO(gh) some kind of entity system, 
    // so that we don't have to store entity_specific things in all of the entities
    RigidBody rb; // TODO(gh) make this a pointer!

    // TODO(gh) CollisionVolumeGroup!
    CollisionVolumeCube cv;

    AABB aabb; 
};

#endif
