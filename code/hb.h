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

    // IMPORTANT(gh) Always make sure that we are recording a certain amount of
    // frames, and it is hard to keep track of new memory allocations 
    // happening within the frames - which means that we should preferably use
    // this arenas as a temp memory or as something that we can use while 
    // initializing thigs.

    RandomSeries random_series;

    PBDParticlePool particle_pool;
};

#define desired_time_machine_seconds 30

// NOTE(gh) Things we don't need to preserve
struct TranState
{
    b32 is_initialized;

    u32 max_pbd_substep_count;
    // NOTE(gh) In realtime, this would be like max_pbd_substep_count in maximum.
    // In time machine, this will be 1 and be replenished when the user presses the key or something.
    i32 remaining_pbd_substep_count;

    GrassGrid *grass_grids;
    u32 grass_grid_count_x;
    u32 grass_grid_count_y;

    // NOTE(gh) rendering related stuffs
    Camera game_camera;
    Camera debug_camera;

    // TODO(gh) Temp thing
    LoadedVOXResult loaded_voxs[64];
    u32 loaded_vox_count;

    // NOTE(gh) Where all non permanent things should go inside
    MemoryArena transient_arena; 

    GameAssets assets;

    GameState *saved_game_states;
    u32 max_saved_game_state_count;
    u32 saved_game_state_read_cursor; // Will be starting from the write cursor, and decrement by 1
    u32 saved_game_state_write_cursor; // When the time machine starts, we will going to start the read cursor from the write cursor
    b32 is_wrapped;
};

#endif
