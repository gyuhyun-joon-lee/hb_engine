/*
 * Written by Gyuhyun Lee
 */

#ifndef HB_ENTITY_H
#define HB_ENTITY_H

enum EntityType
{
    EntityType_Null,
    EntityType_AABB,
    EntityType_Floor,
    EntityType_Sphere,
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

    VertexPN *vertices;
    u32 vertex_count;
    // TODO(gh) We don't really need to hold the indices, 
    // as long as we have the cpu-visible index buffer and the offset.
    // For the ray cast, we can use the same buffer.
    u32 *indices; 
    u32 index_count;

    u32 x_quad_count;
    u32 y_quad_count;

    // TODO(gh) CollisionVolumeGroup!
    CollisionVolumeCube cv;

    AABB aabb; 
};

#endif
