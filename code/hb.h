#ifndef HB_H
#define HB_H

struct GameState
{
    b32 is_initialized;

    Entity *entities;
    u32 entity_count;
    u32 max_entity_count;

    // NOTE(gh) rendering related stuffs
    Camera camera;
    MemoryArena render_arena;

    // NOTE(gh) Where all non permanent things should go inside
    MemoryArena transient_arena; 

    MemoryArena mass_agg_arena;

    // TODO(gh) seed this properly
    RandomSeries random_series;

    // NOTE(gh) voxel related things... need to clean up later
    VoxelWorld voxel_world;
};

#endif
