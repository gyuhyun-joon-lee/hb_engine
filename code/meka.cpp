#include "meka_types.h"
#include "meka_simd.h"
#include "meka_platform.h"
#include "meka_math.h"
#include "meka_random.h"
#include "meka_intrinsic.h"
#include "meka_font.h"
#include "meka_simulation.h"
#include "meka_entity.h"
#include "meka_voxel.h"
#include "meka_render_group.h"
#include "meka.h"

#include "meka_mesh_loader.cpp"
#include "meka_voxel.cpp"
#include "meka_ray.cpp"
#include "meka_simulation.cpp"
#include "meka_entity.cpp"
#include "meka_terrain.cpp"
#include "meka_render_group.cpp"
#include "meka_image_loader.cpp"

extern "C" 
GAME_UPDATE_AND_RENDER(update_and_render)
{
    GameState *game_state = (GameState *)platform_memory->permanent_memory;
    VoxelWorld *world = &game_state->world;
    if(!game_state->is_initialized)
    {
        // TODO(joon) entity arena?
        game_state->max_entity_count = 160000;
        game_state->entities = (Entity *)malloc(sizeof(Entity) * game_state->max_entity_count);

        //PlatformReadFileResult vox_file = platform_api->read_file("/Volumes/meka/meka_renderer/data/vox/chr_knight.vox");
        PlatformReadFileResult vox_file = platform_api->read_file("/Volumes/meka/meka_renderer/data/vox/monu10.vox");
        load_vox_result loaded_vox = load_vox(vox_file.memory, vox_file.size);
        platform_api->free_file_memory(vox_file.memory);

        //add_voxel_entity_from_vox_file(game_state, loaded_vox);
        free_loaded_vox(&loaded_vox);

        game_state->transient_arena = start_memory_arena(platform_memory->transient_memory, megabytes(256));

        game_state->mass_agg_arena = start_memory_arena((u8 *)platform_memory->transient_memory + game_state->transient_arena.total_size, megabytes(256));
        add_cube_mass_agg_entity(game_state, &game_state->mass_agg_arena, V3(0, 0, 10), V3(1.0f, 1.0f, 1.0f), V3(1, 1, 1), 1.0f, 15.0f);
        //add_flat_triangle_mass_agg_entity(game_state, &game_state->mass_agg_arena, V3(1, 1, 1), 1.0f, 15.0f);

        //add_room_entity(game_state, V3(0, 0, 0), V3(100.0f, 100.0f, 100.0f), V3(0.3f, 0.3f, 0.3f));
        add_floor_entity(game_state, V3(0, 0, -0.5f), V3(100.0f, 100.0f, 1.0f), V3(0.7f, 0.7f, 0.7f));

#if 0
        PlatformReadFileResult cow_obj_file = platform_api->read_file("/Volumes/meka/meka_engine/data/low_poly_cow.obj");
        raw_mesh cow_mesh = parse_obj_tokens(&game_state->transient_arena, cow_obj_file.memory, cow_obj_file.size);
        // TODO(joon) : Find out why increasing the elastic value make the entity fly away!!!
        add_mass_agg_entity_from_mesh(game_state, &game_state->mass_agg_arena, V3(0, 5, 30), V3(1, 1, 1), 
                                    cow_mesh.positions, cow_mesh.position_count, cow_mesh.indices, cow_mesh.index_count, V3(0.5f, 0.5f, 0.5f), 5.0f, 15.0f);
#endif

        PlatformReadFileResult oh_obj_file = platform_api->read_file("/Volumes/meka/meka_engine/data/octahedron.obj");
        raw_mesh oh_mesh = parse_obj_tokens(&game_state->transient_arena, oh_obj_file.memory, oh_obj_file.size);
        // TODO(joon) : Find out why increasing the elastic value make the entity fly away!!!
        add_mass_agg_entity_from_mesh(game_state, &game_state->mass_agg_arena, V3(0, 5, 30), V3(1, 1, 1), 
                                    oh_mesh.positions, oh_mesh.position_count, oh_mesh.indices, oh_mesh.index_count, V3(0.5f, 0.5f, 0.5f), 5.0f, 100.0f);
        
        game_state->render_arena = start_memory_arena((u8 *)platform_memory->transient_memory + 
                                                    game_state->transient_arena.total_size + 
                                                    game_state->mass_agg_arena.total_size, 
                                                    megabytes(256));

        game_state->camera.focal_length = 1.0f;
        game_state->camera.p = V3(-10, 0, 5);
#if 0
        // NOTE(joon) camera is looking at the (0, 0, 0)
        // TODO(joon) Does not work if the camera.z = 0, 0, 1
        game_state->camera.x_axis = normalize(cross(V3(0, 0, 1), game_state->camera.z_axis));
        game_state->camera.y_axis = normalize(cross(game_state->camera.z_axis, game_state->camera.x_axis));
#endif

        game_state->is_initialized = true;
    }

    Camera *camera= &game_state->camera;

    f32 camera_rotation_speed = 5.0f * platform_input->dt_per_frame;
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
    }
    if(platform_input->action_right)
    {
        camera->roll -= camera_rotation_speed;
    }

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
    
    // NOTE(joon) update entity start
    for(u32 entity_index = 0;
        entity_index < game_state->entity_count;
        ++entity_index)
    {
        Entity *entity = game_state->entities + entity_index;
        switch(entity->type)
        {
            case Entity_Type_AABB:
            {
                //move_entity(game_state, entity, platform_input->dt_per_frame);
            }break;

            case Entity_Type_Mass_Agg:
            {
                // TODO(joon) make physics not to be dependent to the framerate?
                if(is_entity_flag_set(entity, Entity_Flag_Movable))
                {
                    move_mass_agg_entity(game_state, entity, platform_input->dt_per_frame, platform_input->space);
                }
            }break;
        }
    }

    // NOTE(joon) rendering code start
    RenderGroup render_group = {};
    start_render_group(&render_group, platform_render_push_buffer, &game_state->camera, V3(0, 0, 0));
    
    for(u32 entity_index = 0;
        entity_index < game_state->entity_count;
        ++entity_index)
    {
        Entity *entity = game_state->entities + entity_index;
        switch(entity->type)
        {
            case Entity_Type_Floor:
            case Entity_Type_AABB:
            {
                push_aabb(&render_group, 
                          entity->aabb.p, 2.0f*entity->aabb.half_dim, 
                          entity->color);
            }break;
            case Entity_Type_Mass_Agg:
            {
                for(u32 connection_index = 0;
                        connection_index < entity->mass_agg.connection_count;
                        ++connection_index)
                {
                    PiecewiseMassParticleConnection *connection = entity->mass_agg.connections + connection_index;

                    MassParticle *particle_0 = entity->mass_agg.particles + connection->ID_0;
                    MassParticle *particle_1 = entity->mass_agg.particles + connection->ID_1;
                    push_line(&render_group, particle_0->p, particle_1->p, entity->color);
                }
            }break;
        }
    }
}

