/*
 * Written by Gyuhyun Lee
 */
#include "hb_types.h"
#include "hb_simd.h"
#include "hb_intrinsic.h"
#include "hb_math.h"
#include "hb_random.h"
#include "hb_font.h"
#include "hb_render_group.h"
#include "hb_simulation.h"
#include "hb_entity.h"
#include "hb_asset.h"
#include "hb_platform.h"
#include "hb_debug.h"
#include "hb.h"

#include "hb_mesh_loader.cpp"
#include "hb_ray.cpp"
#include "hb_simulation.cpp"
#include "hb_noise.cpp"
#include "hb_entity.cpp"
#include "hb_render_group.cpp"
#include "hb_image_loader.cpp"
#include "hb_asset.cpp"

// TODO(gh) not a great idea
#include <time.h>

internal void 
output_debug_records(PlatformRenderPushBuffer *platform_render_push_buffer, GameAssets *assets, v2 p);

/*
   TODO(gh)
   - Render Font using one of the graphics API
   - If we are going to use the packed texture, how should we correctly sample from it in the shader?
   */

extern "C" 
GAME_UPDATE_AND_RENDER(update_and_render)
{
    GameState *game_state = (GameState *)platform_memory->permanent_memory;
    if(!game_state->is_initialized)
    {
        assert(platform_render_push_buffer->combined_vertex_buffer && 
                platform_render_push_buffer->combined_index_buffer);

        game_state->transient_arena = start_memory_arena(platform_memory->transient_memory, megabytes(512));

        // TODO(gh) entity arena?
        game_state->max_entity_count = 8192;
        game_state->entities = push_array(&game_state->transient_arena, Entity, game_state->max_entity_count);

        game_state->render_arena = start_memory_arena((u8 *)platform_memory->transient_memory + 
                game_state->transient_arena.total_size + 
                game_state->mass_agg_arena.total_size,
                megabytes(16));

        game_state->game_camera = init_fps_camera(V3(0, 0, 22), 1.0f, 135, 1.0f, 1000.0f);
        game_state->game_camera.pitch += 1.5f;
        game_state->debug_camera = init_fps_camera(V3(0, 0, 22), 1.0f, 135, 0.1f, 10000.0f);
        // Close camera
        // game_state->circle_camera = init_circle_camera(V3(0, 0, 5), V3(0, 0, 0), 5.0f, 135, 0.01f, 10000.0f);
        // Far away camera
        game_state->circle_camera = init_circle_camera(V3(0, 0, 20), V3(0, 0, 0), 20.0f, 135, 0.01f, 10000.0f);
        // Really far away camera
        // game_state->circle_camera = init_circle_camera(V3(0, 0, 50), V3(0, 0, 0), 50.0f, 135, 0.01f, 10000.0f);

        u32 permutations255[] = 
        { 
            151,160,137,91,90,15,                 
            131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,    
            190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
            88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
            77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
            102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
            135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
            5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
            223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
            129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
            251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
            49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
            138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
        };
        // TODO(gh) Better way to handle this?
        game_state->permutations255 = push_array(&game_state->transient_arena, u32, array_count(permutations255));

        for(u32 i = 0;
                i < array_count(permutations255);
                ++i)
        {
            game_state->permutations255[i] = permutations255[i];
        }

        v2 grid_dim = V2(100, 100); // TODO(gh) just temporary, need to 'gather' the floors later
        game_state->grass_grid_count_x = 6;
        game_state->grass_grid_count_y = 4;
        game_state->grass_grids = push_array(&game_state->transient_arena, GrassGrid, game_state->grass_grid_count_x*game_state->grass_grid_count_y);

        v2 floor_left_bottom_p = hadamard(-V2(game_state->grass_grid_count_x/2, game_state->grass_grid_count_y/2), grid_dim);
        for(u32 y = 0;
                y < game_state->grass_grid_count_y;
                ++y)
        {
            for(u32 x = 0;
                    x < game_state->grass_grid_count_x;
                    ++x)
            {
                // TODO(gh) Beware that when you change this value, you also need to change the size of grass instance buffer
                // and the indirect command count(for now)
                u32 grass_on_floor_count_x = 256;
                u32 grass_on_floor_count_y = 256;

                v2 min = floor_left_bottom_p + hadamard(grid_dim, V2(x, y));
                v2 max = min + grid_dim;

                Entity *floor_entity = 
                    add_floor_entity(game_state, &game_state->transient_arena, V3(min, 0), grid_dim, V3(0.25f, 0.1f, 0.04f), grass_on_floor_count_x, grass_on_floor_count_y);

                GrassGrid *grid = game_state->grass_grids + y*game_state->grass_grid_count_x + x;
                init_grass_grid(platform_render_push_buffer, floor_entity, &game_state->random_series, grid, grass_on_floor_count_x, grass_on_floor_count_y, min, max);
            }
        }

        load_game_assets(&game_state->assets, platform_api, platform_render_push_buffer->device);

        game_state->is_initialized = true;
    }

    Camera *game_camera = &game_state->game_camera;
    Camera *debug_camera = &game_state->debug_camera;

    Camera *render_camera = game_camera;
    // render_camera = debug_camera;
    b32 show_perlin_noise_grid = false;

    if(render_camera == debug_camera)
    {
        game_camera->roll += 0.8f * platform_input->dt_per_frame;
    }

    f32 camera_rotation_speed = 3.0f * platform_input->dt_per_frame;

    if(platform_input->action_up)
    {
        render_camera->pitch += camera_rotation_speed;
    }
    if(platform_input->action_down)
    {
        render_camera->pitch -= camera_rotation_speed;
    }
    if(platform_input->action_left)
    {
        render_camera->roll += camera_rotation_speed;
    }
    if(platform_input->action_right)
    {
        render_camera->roll -= camera_rotation_speed;
    }

    v3 camera_dir = get_camera_lookat(render_camera);
    v3 camera_right_dir = get_camera_right(render_camera);
    f32 camera_speed = 30.0f * platform_input->dt_per_frame;
    if(platform_input->move_up)
    {
        render_camera->p += camera_speed*camera_dir;
    }
    if(platform_input->move_down)
    {
        render_camera->p -= camera_speed*camera_dir;
    }
    if(platform_input->move_right)
    {
        render_camera->p += camera_speed*camera_right_dir;
    }
    if(platform_input->move_left)
    {
        render_camera->p += -camera_speed*camera_right_dir;
    }

    // NOTE(gh) update entity start
    for(u32 entity_index = 0;
            entity_index < game_state->entity_count;
            ++entity_index)
    {
        Entity *entity = game_state->entities + entity_index;
        switch(entity->type)
        {
            case EntityType_Floor:
            case EntityType_AABB:
                {
                }break;
        }
    }

    u32 update_perlin_noise_buffer_data_count = game_state->grass_grid_count_x * game_state->grass_grid_count_y;
#if 1
    TempMemory perlin_noise_temp_memory = 
        start_temp_memory(&game_state->transient_arena, 
                sizeof(ThreadUpdatePerlinNoiseBufferData)*update_perlin_noise_buffer_data_count);
    ThreadUpdatePerlinNoiseBufferData *update_perlin_noise_buffer_data = 
        push_array(&perlin_noise_temp_memory, ThreadUpdatePerlinNoiseBufferData, update_perlin_noise_buffer_data_count);
#endif

#if 1
    u32 update_perlin_noise_buffer_data_used_count = 0;
    // TODO(gh) populate the perlin noise buffer in each grid, but should the perlin noise be one giant thing 
    // on top of the map?
    for(u32 grass_grid_index = 0;
            grass_grid_index < game_state->grass_grid_count_x*game_state->grass_grid_count_y;
            ++grass_grid_index)
    {
        GrassGrid *grid = game_state->grass_grids + grass_grid_index;

        ThreadUpdatePerlinNoiseBufferData *data = update_perlin_noise_buffer_data + update_perlin_noise_buffer_data_used_count++;
        data->total_x_count = grid->grass_count_x;
        data->total_y_count = grid->grass_count_y;
        data->start_x = 0;
        data->start_y = 0;
        data->offset_x = game_state->offset_x;
        data->one_past_end_x = grid->grass_count_x;
        data->one_past_end_y = grid->grass_count_x;
        data->time_elasped_from_start = platform_input->time_elasped_from_start;
        data->perlin_noise_buffer = grid->perlin_noise_buffer;
        data->permutations255 = game_state->permutations255;
        thread_work_queue->add_thread_work_queue_item(thread_work_queue, thread_update_perlin_noise_buffer_callback, (void *)data);

        assert(update_perlin_noise_buffer_data_used_count <= update_perlin_noise_buffer_data_count);
    }
#endif

    // NOTE(gh) Frustum cull the grids
    // NOTE(gh) As this is just a conceptual test, it doesn't matter whether the NDC z is 0 to 1 or -1 to 1
    m4x4 view = camera_transform(game_camera);
    m4x4 proj = perspective_projection_near_is_01(game_camera->fov, 
            game_camera->near, 
            game_camera->far, 
            platform_render_push_buffer->width_over_height);

    m4x4 proj_view = proj * view;

    // TODO(gh) Grid z is assumed to be 15(z+floor), and we need to be more conservative on these
    for(u32 grass_grid_index = 0;
            grass_grid_index < game_state->grass_grid_count_x*game_state->grass_grid_count_y;
            ++grass_grid_index)
    {
        GrassGrid *grid = game_state->grass_grids + grass_grid_index;

        f32 z = 15.0f;
        // TODO(gh) This will not work if the grid was big enough to contain the frustum
        // more concrete way would be the seperating axis test
        v3 min = V3(grid->min, 0);
        v3 max = V3(grid->max, z);

        v3 vertices[] = 
        {
            // bottom
            V3(min.x, min.y, min.z),
            V3(min.x, max.y, min.z),
            V3(max.x, min.y, min.z),
            V3(max.x, max.y, min.z),

            // top
            V3(min.x, min.y, max.z),
            V3(min.x, max.y, max.z),
            V3(max.x, min.y, max.z),
            V3(max.x, max.y, max.z),
        };

        // TODO(gh) Re enable this frustum culling
        grid->should_draw = true;
        for(u32 i = 0;
                i < array_count(vertices) && !grid->should_draw;
                ++i)
        {
            // homogeneous p
            v4 hp = proj_view * V4(vertices[i], 1.0f);

            // We are using projection matrix which puts z to 0 to 1
            if((hp.x >= -hp.w && hp.x <= hp.w) &&
                    (hp.y >= -hp.w && hp.y <= hp.w) &&
                    (hp.z >= 0 && hp.z <= hp.w))
            {
                grid->should_draw = true;
                break;
            }
        }
    }

    // NOTE(gh) render entity start
    init_render_push_buffer(platform_render_push_buffer, render_camera, game_camera,
            game_state->grass_grids, game_state->grass_grid_count_x, game_state->grass_grid_count_y, 
            V3(0, 0, 0), true);
    platform_render_push_buffer->enable_shadow = false;
    platform_render_push_buffer->enable_grass_rendering = true;
    platform_render_push_buffer->enable_show_perlin_noise_grid = show_perlin_noise_grid;

    for(u32 entity_index = 0;
        entity_index < game_state->entity_count;
        ++entity_index)
    {
        Entity *entity = game_state->entities + entity_index;
        switch(entity->type)
        {
            case EntityType_Floor:
            case EntityType_AABB:
            {
                push_aabb(platform_render_push_buffer, 
                          entity->p, entity->dim, entity->color, 
                          entity->vertices, entity->vertex_count, entity->indices, entity->index_count, false);
            }break;

            case EntityType_Cube:
            {
                push_cube(platform_render_push_buffer, 
                          entity->p, entity->dim, entity->color, 
                          entity->vertices, entity->vertex_count, entity->indices, entity->index_count, true);
            }break;
        }
    }

    // NOTE(gh) push forward rendering elements
    b32 enable_show_game_camera_frustum = (render_camera != game_camera);
    if(enable_show_game_camera_frustum)
    {
        CameraFrustum game_camera_frustum = {};
        get_camera_frustum(game_camera, &game_camera_frustum, platform_render_push_buffer->width_over_height);

        /*
           NOTE(gh)
           When looked from the camera p
           near
           0     1
           -------
           |     |
           |     |
           -------
           2     3

           far
           4     5
           -------
           |     |
           |     |
           -------
           6     7
        */
        v3 frustum_vertices[] = 
        {
            game_camera_frustum.near[0],
            game_camera_frustum.near[1],
            game_camera_frustum.near[2],
            game_camera_frustum.near[3],

            game_camera_frustum.far[0],
            game_camera_frustum.far[1],
            game_camera_frustum.far[2],
            game_camera_frustum.far[3],
        };

        u32 frustum_indices[] =
        {
            // top
            0, 1, 4,
            1, 5, 4,

            // right
            1, 3, 5,
            3, 7, 5,

            // left
            2, 0, 6,
            0, 4, 6,

            // bottom
            6, 7, 2,
            7, 3, 2,

            // near
            2, 3, 0,
            3, 1, 0,

            //far
            7, 6, 5,
            6, 4, 5,
        };

        push_frustum(platform_render_push_buffer, V3(0, 0.2f, 1), 
                    frustum_vertices, array_count(frustum_vertices),
                    frustum_indices, array_count(frustum_indices));

        v3 line_color = V3(0, 1, 1);

        push_line(platform_render_push_buffer, game_camera_frustum.near[0], game_camera_frustum.near[1], line_color);
        push_line(platform_render_push_buffer, game_camera_frustum.near[1], game_camera_frustum.near[3], line_color);
        push_line(platform_render_push_buffer, game_camera_frustum.near[2], game_camera_frustum.near[3], line_color);
        push_line(platform_render_push_buffer, game_camera_frustum.near[2], game_camera_frustum.near[0], line_color);

        push_line(platform_render_push_buffer, game_camera_frustum.far[0], game_camera_frustum.far[1], line_color);
        push_line(platform_render_push_buffer, game_camera_frustum.far[1], game_camera_frustum.far[3], line_color);
        push_line(platform_render_push_buffer, game_camera_frustum.far[3], game_camera_frustum.far[2], line_color);
        push_line(platform_render_push_buffer, game_camera_frustum.far[2], game_camera_frustum.far[0], line_color);

        push_line(platform_render_push_buffer, game_camera_frustum.near[0], game_camera_frustum.far[0], line_color);
        push_line(platform_render_push_buffer, game_camera_frustum.near[1], game_camera_frustum.far[1], line_color);
        push_line(platform_render_push_buffer, game_camera_frustum.near[2], game_camera_frustum.far[2], line_color);
        push_line(platform_render_push_buffer, game_camera_frustum.near[3], game_camera_frustum.far[3], line_color);
    }

    thread_work_queue->complete_all_thread_work_queue_items(thread_work_queue);

    end_temp_memory(&perlin_noise_temp_memory);

#if 1
    char *string = "Let's see if we can render this";

#endif

    // TODO(gh) simplify variables that can effect the wind 
    // maybe with world unit speed
    game_state->time_until_offset_x_inc += platform_input->dt_per_frame;
    if(game_state->time_until_offset_x_inc > 0.02f)
    {
        game_state->offset_x++;
        game_state->time_until_offset_x_inc = 0;
    }

    // TODO(gh) This prevents us from timing the game update and render loop itself 
    output_debug_records(platform_render_push_buffer, &game_state->assets, V2(-0.95f, 0.95f));
}

internal void
debug_text_line(PlatformRenderPushBuffer *platform_render_push_buffer, GameAssets *assets, const char *text, v2 p)
{
    const char *c=text;
    while(*c != '\0')
    {
        f32 x_advance_in_ndc = 
            push_char(platform_render_push_buffer, assets, V3(1, 1, 1), p, (u8)*c);

        p.x += x_advance_in_ndc;
        c++;
    }
}

// NOTE(gh) This counter will fire at the last seconds, meaning this array will be large enough
// to contain all the records that we time_blocked in other codes
DebugRecord game_debug_records[__COUNTER__];

#include <stdio.h>
internal void
output_debug_records(PlatformRenderPushBuffer *platform_render_push_buffer, GameAssets *assets, v2 p)
{
#if HB_DEBUG
    for(u32 record_index = 0;
            record_index < array_count(game_debug_records);
            ++record_index)
    {
        DebugRecord *record = game_debug_records + record_index;
        if(record->function)
        {
            const char *function = record->function;
            const char *file = record->file;
            u32 line = record->line;
            u32 hit_count = record->hit_count_cycle_count >> 32;
            u32 cycle_count = (u32)(record->hit_count_cycle_count & 0xffffffff);

            char buffer[512] = {};
            snprintf(buffer, array_count(buffer),
                    "%s(%s(%u)): %ucy, %uh, %ucy/h ", function, file, line, cycle_count, hit_count, cycle_count/hit_count);

            debug_text_line(platform_render_push_buffer, assets, buffer, p);

            atomic_exchange(&record->hit_count_cycle_count, 0);
            p.y -= 100.0f/platform_render_push_buffer->window_height;
        }
    }
#endif
}



