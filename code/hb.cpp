#include "hb_types.h"
#include "hb_simd.h"
#include "hb_intrinsic.h"
#include "hb_platform.h"
#include "hb_math.h"
#include "hb_random.h"
#include "hb_font.h"
#include "hb_simulation.h"
#include "hb_entity.h"
#include "hb_voxel.h"
#include "hb_render_group.h"
#include "hb.h"

#include "hb_mesh_loader.cpp"
#include "hb_voxel.cpp"
#include "hb_ray.cpp"
#include "hb_simulation.cpp"
#include "hb_entity.cpp"
#include "hb_terrain.cpp"
#include "hb_render_group.cpp"
#include "hb_image_loader.cpp"

/*
    TODO(joon)
    - Render Font using one of the graphics API
        - If we are going to use the packed texture, how should we correctly sample from it in the shader?
*/

extern "C" 
GAME_UPDATE_AND_RENDER(update_and_render)
{
    GameState *game_state = (GameState *)platform_memory->permanent_memory;
    VoxelWorld *world = &game_state->world;
    if(!game_state->is_initialized)
    {
        // TODO(joon) entity arena?
        game_state->max_entity_count = 1024;
        game_state->entities = (Entity *)malloc(sizeof(Entity) * game_state->max_entity_count);

        //PlatformReadFileResult vox_file = platform_api->read_file("/Volumes/hb/hb_renderer/data/vox/chr_knight.vox");
        PlatformReadFileResult vox_file = platform_api->read_file("/Volumes/hb/hb_renderer/data/vox/monu10.vox");
        load_vox_result loaded_vox = load_vox(vox_file.memory, vox_file.size);
        platform_api->free_file_memory(vox_file.memory);

        //add_voxel_entity_from_vox_file(game_state, loaded_vox);
        free_loaded_vox(&loaded_vox);

        game_state->transient_arena = start_memory_arena(platform_memory->transient_memory, megabytes(256));

        game_state->mass_agg_arena = start_memory_arena((u8 *)platform_memory->transient_memory + game_state->transient_arena.total_size, megabytes(256));
        //add_flat_triangle_mass_agg_entity(game_state, &game_state->mass_agg_arena, V3(1, 1, 1), 1.0f, 15.0f);

        //add_room_entity(game_state, V3(0, 0, 0), V3(100.0f, 100.0f, 100.0f), V3(0.3f, 0.3f, 0.3f));
        add_floor_entity(game_state, V3(0, 0, -0.5f), V3(100.0f, 100.0f, 1.0f), V3(0.8f, 0.1f, 0.1f));

        add_cube_rigid_body_entity(game_state, V3(0, 0, 30.0f), V3(1.0f, 1.0f, 1.0f), 3.0f, V3(1.0f, 1.0f, 1.0f));
#if 0
        add_cube_mass_agg_entity(game_state, &game_state->mass_agg_arena, V3(0, 0, 10), V3(1.0f, 1.0f, 1.0f), V3(1, 1, 1), 1.0f, 7.0f);
        PlatformReadFileResult cow_obj_file = platform_api->read_file("/Volumes/hb/hb_engine/data/low_poly_cow.obj");
        raw_mesh cow_mesh = parse_obj_tokens(&game_state->transient_arena, cow_obj_file.memory, cow_obj_file.size);
        // TODO(joon) : Find out why increasing the elastic value make the entity fly away!!!
        add_mass_agg_entity_from_mesh(game_state, &game_state->mass_agg_arena, V3(0, 5, 30), V3(1, 1, 1), 
                                    cow_mesh.positions, cow_mesh.position_count, cow_mesh.indices, cow_mesh.index_count, V3(0.001f, 0.001f, 0.001f), 5.0f, 0.1f);
        PlatformReadFileResult oh_obj_file = platform_api->read_file("/Volumes/hb/hb_engine/data/dodecahedron.obj");
        raw_mesh oh_mesh = parse_obj_tokens(&game_state->transient_arena, oh_obj_file.memory, oh_obj_file.size);
        // TODO(joon) : Find out why increasing the elastic value make the entity fly away!!!
        add_mass_agg_entity_from_mesh(game_state, &game_state->mass_agg_arena, V3(0, 5, 30), V3(1, 1, 1), 
                                    oh_mesh.positions, oh_mesh.position_count, oh_mesh.indices, oh_mesh.index_count, V3(0.5f, 0.5f, 0.5f), 5.0f, 10.0f);
#endif
        
        game_state->render_arena = start_memory_arena((u8 *)platform_memory->transient_memory + 
                                                    game_state->transient_arena.total_size + 
                                                    game_state->mass_agg_arena.total_size,
                                                    megabytes(128));

        game_state->random_series = start_random_series(123123);

        game_state->camera = init_camera(V3(-10, 0, 5), V3(0, 0, 0), 1.0f);
        
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

    /*
        NOTE(joon) How we are going to update the entities (without the friction)
        - move the entities, without thinking about the interpenetration
            - knowing where the first collision p can be done, but very expensive
        - generate collision data
            - to avoid oop, we use the good-old push buffer, with header(entities, friction, restitution, data count),
              followed by the data(contact point, normal, and penetration depth)
        - resolve interpenetration
        - resolve collision
    */
    
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

            case Entity_Type_Cube:
            {
                RigidBody *rb = &entity->rb;
                // NOTE(joon) Always should happen for all rigid bodies at the start of their update
                init_rigid_body_and_calculate_derived_parameters_for_frame(rb);

                if(!compare_with_epsilon(rb->inv_mass, 0.0f))
                {
                    // TODO(joon) This is a waste of time, as we are going to negate the mass portion anyway
                    rb->force += V3(0, 0, -9.8f) / rb->inv_mass;
                }

                v3 ddp = (rb->inv_mass * rb->force);
            
                rb->dp += platform_input->dt_per_frame * ddp;

                rb->p += platform_input->dt_per_frame*rb->dp;

                v3 angular_ddp = rb->transform_inv_inertia_tensor * rb->torque;
                rb->angular_dp += platform_input->dt_per_frame * angular_ddp;
                // NOTE(joon) equation here is orientation += 0.5f*dt*angular_ddp*orientation
                rb->orientation += 0.5f*
                                   platform_input->dt_per_frame*
                                   Quat(0, rb->angular_dp) *
                                   rb->orientation;
                assert(is_pure_quat(rb->orientation));

                // NOTE(joon) clear computed parameters
                // TODO(joon) would be nice if we can put this in intialize rb function,
                entity->rb.force = V3();
                entity->rb.torque = V3();
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

            case Entity_Type_Cube:
            {
                push_cube(&render_group, entity->rb.p, entity->rb.dim, entity->color, entity->rb.orientation);
            }break;
        }
    }
}

