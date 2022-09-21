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
#include "hb_entity.cpp"
#include "hb_terrain.cpp"
#include "hb_render_group.cpp"
#include "hb_image_loader.cpp"

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
        game_state->transient_arena = start_memory_arena(platform_memory->transient_memory, megabytes(256));

        // TODO(gh) entity arena?
        game_state->max_entity_count = 1024;
        game_state->entities = push_array(&game_state->transient_arena, Entity, game_state->max_entity_count);
        
        game_state->render_arena = start_memory_arena((u8 *)platform_memory->transient_memory + 
                                                    game_state->transient_arena.total_size + 
                                                    game_state->mass_agg_arena.total_size,
                                                    megabytes(128));

        game_state->random_series = start_random_series(123123);

        game_state->camera = init_camera(V3(0, 0, 30), V3(0, 0, 0), 1.0f);
        game_state->circle_camera = init_circle_camera(V3(0, 0, 10), V3(0, 0, 0), 10.0f, 135, 0.01f, 10000.0f);

        // add_floor_entity(game_state, V3(0, 0, 2), V3(2, 2, 2), V3(1, 1, 1));
        add_floor_entity(game_state, &game_state->transient_arena, V3(0, 0, -1), V3(10, 10, 2), V3(1, 1, 1));

        add_grass_entity(game_state, platform_render_push_buffer, &game_state->transient_arena, 
                         V3(0, 0, 0), 0.2f, V3(0, 1, 0.2f),
                         V2(1, 0), 1.0f, 0.5f);
        
        game_state->is_initialized = true;
    }

    Camera *camera= &game_state->camera;
    f32 camera_rotation_speed = 2.0f * platform_input->dt_per_frame;

    if(platform_input->action_up)
    {
        // camera->pitch += camera_rotation_speed;
    }
    if(platform_input->action_down)
    {
        // camera->pitch -= camera_rotation_speed;
    }
    if(platform_input->action_left)
    {
        game_state->circle_camera.rad -= 0.6f * platform_input->dt_per_frame;
    }
    if(platform_input->action_right)
    {
        game_state->circle_camera.rad += 0.6f * platform_input->dt_per_frame;
    }

    v3 camera_dir = get_camera_lookat(camera);
    f32 camera_speed = 10.0f * platform_input->dt_per_frame;
    if(platform_input->move_up)
    {
        // camera->p += camera_speed*camera_dir;
    }
    if(platform_input->move_down)
    {
        // camera->p -= camera_speed*camera_dir;
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
                local_persist f32 t = 0.0f;

                // entity->tilt_direction = V2(cosf(t), sinf(t));
                entity->width = 5.0f;
                entity->tilt = 4.0f;
                populate_grass_vertices(V3(0, 0, 0), entity->width, entity->tilt_direction, entity->tilt, entity->bend, entity->grass_divided_count, 
                                        entity->vertices, entity->vertex_count);

                t += platform_input->dt_per_frame;
            }break;
        }
    }

    // NOTE(gh) render entity start
    init_render_push_buffer(platform_render_push_buffer, &game_state->circle_camera, V3(0, 0, 0), true);

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
                          entity->vertices, entity->vertex_count, entity->indices, entity->index_count);
            }break;

            case EntityType_Grass:
            {
                push_grass(platform_render_push_buffer, entity->p, entity->dim, entity->color, 
                            entity->vertices, entity->vertex_count, entity->indices, entity->index_count);
            }break;
        }
    }
}

