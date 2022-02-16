#ifndef MEKA_ENTITY_H
#define MEKA_ENTITY_H

enum EntityType
{
    entity_type_null,
    entity_type_voxel,
    entity_type_bullet,
    entity_type_orc,
    entity_type_goblin,
    entity_type_room,
};

struct Entity
{
    EntityType type;
    v3 p;
    v3 v;
    v3 a;

    // TODO(joon) This really should not be here..
    // or we can have a palette that is relatve to this entity, similiar to teardown engine
    u32 color;

    // NOTE(joon) collision volume is always a cube(for now)
    // TODO(joon) collision group & volume
    v3 dim;
};

#endif
