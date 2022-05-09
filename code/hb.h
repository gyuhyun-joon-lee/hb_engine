#ifndef HB_H
#define HB_H

struct GameState
{
    b32 is_initialized;

    Entity *entities;
    u32 entity_count;
    u32 max_entity_count;

    // NOTE(joon) rendering related stuffs
    Camera camera;
    MemoryArena render_arena;

    // NOTE(joon) voxel related stuffs
    VoxelWorld world;
    MemoryArena voxel_arena;

    // NOTE(joon) Where all non permanent things should go inside
    MemoryArena transient_arena; 

    MemoryArena mass_agg_arena;

    // TODO(joon) seed this properly
    RandomSeries random_series;
};

#endif
