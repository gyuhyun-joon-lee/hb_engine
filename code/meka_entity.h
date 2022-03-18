#ifndef MEKA_ENTITY_H
#define MEKA_ENTITY_H

enum EntityType
{
    Entity_Type_Null,
    Entity_Type_Voxel,
    Entity_Type_Bullet,
    Entity_Type_Room,
};

struct Entity
{
    EntityType type;
    v3 p;
    v3 v;
    v3 a;

    f32 mass; 

    // TODO(joon) move this to the collision group
    AABB aabb;

    // TODO(joon) For the voxels, we can have a palette that is relatve to this entity, similiar to the teardown engine
    v3 color;

    // NOTE(joon) collision volume is always a cube(for now)
    // TODO(joon) collision group & volume
    v3 dim;
};

#endif
