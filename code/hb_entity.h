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
    EntityFlag_Null = 0,
    EntityFlag_Movable = 2,
    EntityFlag_Collides = 4,
};

// TODO(gh) For some entities such as rigid body or pbd based entities
// this structure _useless_
struct GenericEntityInfo
{
    v3 position;
    v3 dim;
};

struct Entity
{
    u32 ID;
    EntityType type;
    u32 flags;

    GenericEntityInfo generic_entity_info;

    v3 color;

    b32 should_cast_shadow;

    // TODO(gh) Don't need this for all of the entities, too... 
    PBDParticleGroup particle_group;
};

#endif
