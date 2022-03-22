#ifndef MEKA_H
#define MEKA_H

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

    MemoryArena mass_agg_arena;
};

#endif
