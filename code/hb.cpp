/*
 * Written by Gyuhyun Lee
 */
#include "hb_types.h"
#include "hb_simd.h"
#include "hb_intrinsic.h"
#include "hb_platform.h"
#include "hb_math.h"
#include "hb_random.h"
#include "hb_font.h"
#include "hb_simulation.h"
#include "hb_entity.h"
#include "hb_render_group.h"
#include "hb.h"

#include "hb_mesh_loader.cpp"
#include "hb_ray.cpp"
#include "hb_simulation.cpp"
#include "hb_noise.cpp"
#include "hb_entity.cpp"
#include "hb_render_group.cpp"
#include "hb_image_loader.cpp"

// TODO(gh) not a great idea
#include <time.h>

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

        game_state->transient_arena = start_memory_arena(platform_memory->transient_memory, megabytes(256));

        // TODO(gh) entity arena?
        game_state->max_entity_count = 8192;
        game_state->entities = push_array(&game_state->transient_arena, Entity, game_state->max_entity_count);
        
        game_state->render_arena = start_memory_arena((u8 *)platform_memory->transient_memory + 
                                                    game_state->transient_arena.total_size + 
                                                    game_state->mass_agg_arena.total_size,
                                                    megabytes(128));

        // TODO(gh) get rid of rand(), or get the time and rand information from platform layer?
        srand(time(0));
        game_state->random_series = start_random_series(rand());

        game_state->camera = init_fps_camera(V3(100, 100, 22), 1.0f, 135, 0.01f, 10000.0f);
        // Close camera
        // game_state->circle_camera = init_circle_camera(V3(0, 0, 5), V3(0, 0, 0), 5.0f, 135, 0.01f, 10000.0f);
        // Far away camera
        game_state->circle_camera = init_circle_camera(V3(0, 0, 20), V3(0, 0, 0), 20.0f, 135, 0.01f, 10000.0f);
        // Really far away camera
        // game_state->circle_camera = init_circle_camera(V3(0, 0, 50), V3(0, 0, 0), 50.0f, 135, 0.01f, 10000.0f);

        // add_cube_entity(game_state, V3(0, 0, 15), V3(7, 7, 7), V3(1, 1, 1));

        v2 combined_floor_dim = V2(200, 200); // TODO(gh) just temporary, need to 'gather' the floors later
        game_state->grass_grid_count_x = 2;
        game_state->grass_grid_count_y = 2;
        game_state->grass_grids = push_array(&game_state->transient_arena, GrassGrid, game_state->grass_grid_count_x*game_state->grass_grid_count_y);
        v2 sub_floor_dim = V2(combined_floor_dim.x/game_state->grass_grid_count_x, combined_floor_dim.y/game_state->grass_grid_count_y); // Floor is only consisted of the flat triangles

        v2 floor_left_bottom_p = 0.5f*combined_floor_dim;
        for(u32 y = 0;
                y < game_state->grass_grid_count_y;
                ++y)
        {
            for(u32 x = 0;
                    x < game_state->grass_grid_count_x;
                    ++x)
            {
                u32 grass_on_floor_count_x = 256;
                u32 grass_on_floor_count_y = 256;

                v2 min = floor_left_bottom_p + hadamard(sub_floor_dim, V2(x, y));
                v2 max = min + sub_floor_dim;
                v2 center = 0.5f*(min + max);

                Entity *floor_entity = 
                    add_floor_entity(game_state, &game_state->transient_arena, V3(center, 0), sub_floor_dim, V3(1, 1, 1), grass_on_floor_count_x, grass_on_floor_count_y);

                GrassGrid *grid = game_state->grass_grids + y*game_state->grass_grid_count_x + x;
                init_grass_grid(platform_render_push_buffer, floor_entity, &game_state->random_series, grid, grass_on_floor_count_x, grass_on_floor_count_y, min, max);
            }
        }

#if 0
        u32 desired_grass_count = 5000;
        // NOTE(gh) grass entities would always be sequential to each other.
        plant_grasses_using_white_noise(game_state, &game_state->random_series, platform_render_push_buffer, &game_state->transient_arena, 
                                        floor_width, floor_height, 0, desired_grass_count);
#else 
#if 0
        u32 desired_grass_count = 100;
        plant_grasses_using_brute_force_blue_noise(game_state, &game_state->random_series, platform_render_push_buffer, &game_state->transient_arena, 
                                                  10, 10, 0, desired_grass_count, 0.1f);
#endif
#endif
        
        game_state->is_initialized = true;
    }

    Camera *camera= &game_state->camera;
    f32 camera_rotation_speed = 2.0f * platform_input->dt_per_frame;

    if(platform_input->action_up)
    {
        camera->pitch += camera_rotation_speed;
    }
    if(platform_input->action_down)
    {
        camera->pitch -= camera_rotation_speed;
    }
    if(platform_input->action_left)
    {
        camera->roll += camera_rotation_speed;
        game_state->circle_camera.rad -= 0.6f * platform_input->dt_per_frame;
    }
    if(platform_input->action_right)
    {
        camera->roll -= camera_rotation_speed;
        game_state->circle_camera.rad += 0.6f * platform_input->dt_per_frame;
    }

    v3 camera_dir = get_camera_lookat(camera);
        // TODO(gh) Up vector might be same as the camera direction
    v3 camera_right_dir = normalize(cross(camera_dir, V3(1, 0, 0)));
    f32 camera_speed = 20.0f * platform_input->dt_per_frame;
    if(platform_input->move_up)
    {
        camera->p += camera_speed*camera_dir;
    }
    if(platform_input->move_down)
    {
        camera->p -= camera_speed*camera_dir;
    }
    if(platform_input->move_right)
    {
        camera->p += camera_speed*camera_right_dir;
    }
    if(platform_input->move_left)
    {
        camera->p += -camera_speed*camera_right_dir;
    }

    game_state->circle_camera.p.x = game_state->circle_camera.distance_from_axis * 
                                    cosf(game_state->circle_camera.rad);
    game_state->circle_camera.p.y = game_state->circle_camera.distance_from_axis * 
                                    sinf(game_state->circle_camera.rad);
    
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

            case EntityType_Grass:
            {
                f32 wiggliness = 3.1f;
                // so this is more like where I test some configurations with a small number of grasses 
                populate_grass_vertices(entity->p, entity->blade_width, entity->stride, entity->height, entity->tilt_direction, entity->tilt, entity->bend, entity->grass_divided_count, 
                                        entity->vertices, entity->vertex_count, entity->hash, platform_input->time_elapsed_from_start, wiggliness);
            }break;
        }
    }

    u32 update_perlin_noise_buffer_data_count = game_state->grass_grid_count_x * game_state->grass_grid_count_y;
    TempMemory perlin_noise_temp_memory = 
        start_temp_memory(&game_state->transient_arena, 
                        sizeof(ThreadUpdatePerlinNoiseBufferData)*update_perlin_noise_buffer_data_count);
    ThreadUpdatePerlinNoiseBufferData *update_perlin_noise_buffer_data = 
        push_array(&perlin_noise_temp_memory, ThreadUpdatePerlinNoiseBufferData, update_perlin_noise_buffer_data_count);

    u32 update_perlin_noise_buffer_data_used_count = 0;
    // TODO(gh) populate the perlin noise buffer in each grid, should be a better way to handle this?
    for(u32 grass_grid_index = 0;
            grass_grid_index < game_state->grass_grid_count_x*game_state->grass_grid_count_y;
            ++grass_grid_index)
    {
        GrassGrid *grid = game_state->grass_grids + grass_grid_index;

        ThreadUpdatePerlinNoiseBufferData *data = update_perlin_noise_buffer_data + update_perlin_noise_buffer_data_used_count++;
        data->total_x_count = grid->grass_count_x;
        data->total_y_count = grid->grass_count_y;
        // TODO(gh) Should be continuous, think about how we'll achieve it later down the road
        data->start_x = 0;
        data->start_y = 0;
        data->offset_x = game_state->offset_x;
        data->one_past_end_x = grid->grass_count_x;
        data->one_past_end_y = grid->grass_count_x;
        data->time_elapsed_from_start = platform_input->time_elapsed_from_start;
        data->perlin_noise_buffer_memory = grid->perlin_noise_buffer;
        thread_work_queue->add_thread_work_queue_item(thread_work_queue, thread_update_perlin_noise_buffer_callback, (void *)data);

        assert(update_perlin_noise_buffer_data_used_count <= update_perlin_noise_buffer_data_count);
    }

    // NOTE(gh) render entity start
    // init_render_push_buffer(platform_render_push_buffer, &game_state->circle_camera, V3(0, 0, 0), true);
    init_render_push_buffer(platform_render_push_buffer, &game_state->camera, 
                            game_state->grass_grids, game_state->grass_grid_count_x, game_state->grass_grid_count_y, 
                            V3(0, 0, 0), true);
    platform_render_push_buffer->enable_shadow = false;
    platform_render_push_buffer->enable_grass_mesh_rendering = true;
    platform_render_push_buffer->enable_show_perlin_noise_grid = true;

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

            case EntityType_Grass:
            {
                v3 color = entity->color;

                push_grass(platform_render_push_buffer, entity->p, entity->dim, color, 
                            entity->vertices, entity->vertex_count, entity->indices, entity->index_count, false);
            }break;
        }
    }

    thread_work_queue->complete_all_thread_work_queue_items(thread_work_queue);

    end_temp_memory(&perlin_noise_temp_memory);

    game_state->time_until_offset_x_inc += platform_input->dt_per_frame;

    if(game_state->time_until_offset_x_inc > 0.02f)
    {
        game_state->offset_x++;
        game_state->time_until_offset_x_inc = 0;
    }
}

