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
#include "hb_shared_with_shader.h"
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
                platform_render_push_buffer->combined_index_buffer &&
                platform_render_push_buffer->floor_z_buffer);

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

        game_state->camera = init_fps_camera(V3(0, 0, 22), 1.0f, 135, 0.01f, 10000.0f);
        // Close camera
        // game_state->circle_camera = init_circle_camera(V3(0, 0, 5), V3(0, 0, 0), 5.0f, 135, 0.01f, 10000.0f);
        // Far away camera
        game_state->circle_camera = init_circle_camera(V3(0, 0, 20), V3(0, 0, 0), 20.0f, 135, 0.01f, 10000.0f);
        // Really far away camera
        // game_state->circle_camera = init_circle_camera(V3(0, 0, 50), V3(0, 0, 0), 50.0f, 135, 0.01f, 10000.0f);

        // add_cube_entity(game_state, V3(0, 0, 15), V3(7, 7, 7), V3(1, 1, 1));

        v2 floor_dim = V2(200, 200); // Floor is only consisted of the flat triangles
        Entity *floor_entity = add_floor_entity(game_state, &game_state->transient_arena, V3(0, 0, 0), floor_dim, V3(1, 1, 1), grass_per_grid_count_x, grass_per_grid_count_y);

        raycast_to_populate_floor_z_buffer(floor_entity, platform_render_push_buffer->floor_z_buffer);

#if 0
        u32 desired_grass_count = 5000;
        // NOTE(gh) grass entities would always be sequential to each other.
        plant_grasses_using_white_noise(game_state, &game_state->random_series, platform_render_push_buffer, &game_state->transient_arena, 
                                        floor_width, floor_height, 0, desired_grass_count);
#else 
#if 1
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

    b32 is_space_pressed = false;
    v3 camera_dir = get_camera_lookat(camera);
    f32 camera_speed = 10.0f * platform_input->dt_per_frame;
    if(platform_input->move_up)
    {
        camera->p += camera_speed*camera_dir;
    }
    if(platform_input->move_down)
    {
        camera->p -= camera_speed*camera_dir;
    }
    if(platform_input->space)
    {
        is_space_pressed = true;
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
                if(is_space_pressed)
                {
                    wiggliness = 5.0f;
                }
                // so this is more like where I test some configurations with a small number of grasses 
                populate_grass_vertices(entity->p, entity->blade_width, entity->stride, entity->height, entity->tilt_direction, entity->tilt, entity->bend, entity->grass_divided_count, 
                                        entity->vertices, entity->vertex_count, entity->hash, platform_input->time_elapsed_from_start, wiggliness);
            }break;
        }
    }

    // NOTE(gh) render entity start
    // init_render_push_buffer(platform_render_push_buffer, &game_state->circle_camera, V3(0, 0, 0), true);
    init_render_push_buffer(platform_render_push_buffer, &game_state->camera, V3(0, 0, 0), true);
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
                if(is_space_pressed)
                {
                    color = V3(1, 0, 0);
                }

                push_grass(platform_render_push_buffer, entity->p, entity->dim, color, 
                            entity->vertices, entity->vertex_count, entity->indices, entity->index_count, false);
            }break;
        }
    }
}

