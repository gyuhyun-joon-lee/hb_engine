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
    CircleCamera circle_camera;

    MemoryArena render_arena;

    // NOTE(gh) Where all non permanent things should go inside
    MemoryArena transient_arena; 

    MemoryArena mass_agg_arena;

    // TODO(gh) seed this properly
    RandomSeries random_series;

    f32 accumulated_dt; // time from the game started
    // TODO(gh) More structured light information
};

#endif
