#ifndef MEKA_ENTITY_H
#define MEKA_ENTITY_H

enum EntityType
{
    Entity_Type_Null,
    Entity_Type_AABB,
    Entity_Type_Floor,
    Entity_Type_Mass_Agg,
};

enum EntityFlag
{
    Entity_Flag_Null = 0,
    Entity_Flag_Movable = 2,
    Entity_Flag_Collides = 4,
};

struct Entity
{
    EntityType type;
    u32 flags;

    // TODO(joon) For the voxels, we can have a palette that is relatve to this entity, similiar to the teardown engine
    v3 color;

    // TODO(joon) some kind of entity system, 
    // so that we don't have to store entity_specific things in all of the entities
    MassAgg mass_agg;
    AABB aabb; 
};

#endif
