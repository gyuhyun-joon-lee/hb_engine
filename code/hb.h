/*
 * Written by Gyuhyun Lee
 */

#ifndef HB_H
#define HB_H

// NOTE(gh) Things we have to preserve for the time machine
struct GameState
{
    b32 is_initialized;

    Entity entities[512];
    u32 entity_count;

    // NOTE(gh) rendering related stuffs
    Camera game_camera;
    Camera debug_camera;
    CircleCamera circle_camera;

    // IMPORTANT(gh) Always make sure that we are recording a certain amount of
    // frames, and it is hard to keep track of new memory allocations 
    // happening within the frames - which means that we should preferably use
    // this arenas as a temp memory or as something that we can use while 
    // initializing thigs.
    MemoryArena render_arena;
    MemoryArena entity_arena;

    RandomSeries random_series;

    PBDParticlePool particle_pool;
};

// NOTE(gh) Things we don't need to preserve
struct TranState
{
    b32 is_initialized;

    GrassGrid *grass_grids;
    u32 grass_grid_count_x;
    u32 grass_grid_count_y;

    // TODO(gh) Temp thing
    LoadedVOXResult loaded_voxs[64];
    u32 loaded_vox_count;

    // NOTE(gh) Where all non permanent things should go inside
    MemoryArena transient_arena; 

    GameAssets assets;
};

#endif
