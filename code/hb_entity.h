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

    EntityType_PBD,
};

enum EntityFlag
{
    EntityFlag_Null = (1<<0),
    EntityFlag_Movable = (1<<1),
    EntityFlag_Collides = (1<<2),
    // NOTE(gh) Shape matching properties,
    // these flags will produce different rigid body deformation matrices
    EntityFlag_RigidBody = (1<<3),
    EntityFlag_Linear = (1<<4),
    EntityFlag_Quadratic = (1<<4),
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

    // TODO(gh) We need this, because some entities are not retrievable 
    // only using the asset tag(i.e softbody vertices, their vertices 
    // slightly differ from each other). This ID will get populated at the
    // start of the game for now, but we can make this so that static entities 
    // populate this asset just as we were doing before(when we were rendering),
    // and only populate the dynamic entites like softbody entities
    u32 mesh_assetID;

    GenericEntityInfo generic_entity_info;
    v3 render_dim;

    v3 color;

    // TODO(gh) Don't need this for all of the entities, too... 
    PBDParticleGroup particle_group;
};

#endif
