/*
 * Written by Gyuhyun Lee
 */

#ifndef HB_H
#define HB_H

struct GameState
{
    b32 is_initialized;

    // TODO(gh) Temp thing
    LoadedVOXResult loaded_voxs[64];
    u32 loaded_vox_count;

    Entity *entities;
    u32 entity_count;
    u32 max_entity_count;

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
    // NOTE(gh) Where all non permanent things should go inside
    MemoryArena transient_arena; 

    RandomSeries random_series;

    GrassGrid *grass_grids;
    u32 grass_grid_count_x;
    u32 grass_grid_count_y;

    FluidCubeMAC fluid_cube_mac;

    GameAssets assets;

    PBDParticlePool particle_pool;

    // TODO(gh) More structured light information

    // TODO(gh) some sort of asset system
};

#endif
