/*
 * Written by Gyuhyun Lee
 */

#ifndef HB_H
#define HB_H

struct GameState
{
    b32 is_initialized;

    Entity *entities;
    u32 entity_count;
    u32 max_entity_count;

    // NOTE(gh) rendering related stuffs
    Camera game_camera;
    Camera debug_camera;
    CircleCamera circle_camera;

    MemoryArena render_arena;
    // NOTE(gh) Where all non permanent things should go inside
    MemoryArena transient_arena; 
    MemoryArena mass_agg_arena;

    RandomSeries random_series;

    GrassGrid *grass_grids;
    u32 grass_grid_count_x;
    u32 grass_grid_count_y;

    u32 offset_x;
    f32 time_until_offset_x_inc;

    GameAssets assets;

    TextureAsset vector_field;

    // TODO(gh) More structured light information

    // TODO(gh) some sort of asset system
};

#endif
