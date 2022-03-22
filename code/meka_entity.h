#ifndef MEKA_ENTITY_H
#define MEKA_ENTITY_H

enum EntityType
{
    Entity_Type_Null,
    Entity_Type_Voxel,
    Entity_Type_Bullet,
    Entity_Type_Room,
    Entity_Type_Mass_Agg,
};

struct Entity
{
    EntityType type;

    // TODO(joon) some kind of entity system, so that we don't have to store this
    // for every entity.
    MassAgg mass_agg;

    // TODO(joon) For the voxels, we can have a palette that is relatve to this entity, similiar to the teardown engine
    v3 color;
};

#endif
