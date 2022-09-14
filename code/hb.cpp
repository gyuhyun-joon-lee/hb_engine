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
        // TODO(gh) entity arena?
        game_state->max_entity_count = 1024;
        game_state->entities = (Entity *)malloc(sizeof(Entity) * game_state->max_entity_count);

        game_state->transient_arena = start_memory_arena(platform_memory->transient_memory, megabytes(256));
        
        game_state->render_arena = start_memory_arena((u8 *)platform_memory->transient_memory + 
                                                    game_state->transient_arena.total_size + 
                                                    game_state->mass_agg_arena.total_size,
                                                    megabytes(128));

        game_state->random_series = start_random_series(123123);

        game_state->camera = init_camera(V3(0, 0, 30), V3(0, 0, 0), 1.0f);

        add_floor_entity(game_state, V3(0, 0, 0), V3(10, 10, 10), V3(1, 1, 1));
        
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
        NOTE(gh) How we are going to update the entities (without the friction)
        - move the entities, without thinking about the interpenetration
            - knowing where the first collision p can be done, but very expensive
        - generate collision data
            - to avoid oop, we use the good-old push buffer, with header(entities, friction, restitution, data count),
              followed by the data(contact point, normal, and penetration depth)
        - resolve interpenetration
        - resolve collision
    */
    
    // NOTE(gh) update entity start
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
                // TODO(gh) make physics not to be dependent to the framerate?
                if(is_entity_flag_set(entity, Entity_Flag_Movable))
                {
                    move_mass_agg_entity(game_state, entity, platform_input->dt_per_frame, platform_input->space);
                }
            }break;

            case Entity_Type_Cube:
            {
                RigidBody *rb = &entity->rb;
                // NOTE(gh) Always should happen for all rigid bodies at the start of their update
                init_rigid_body_and_calculate_derived_parameters_for_frame(rb);

#if 0
                if(!compare_with_epsilon(rb->inv_mass, 0.0f))
                {
                    // TODO(gh) This is a waste of time, as we are going to negate the mass portion anyway
                    rb->force += V3(0, 0, -9.8f) / rb->inv_mass;
                }
#endif

                v3 ddp = (rb->inv_mass * rb->force);
            
                rb->dp += platform_input->dt_per_frame * ddp;

                rb->p += platform_input->dt_per_frame*rb->dp;

                v3 angular_ddp = rb->transform_inv_inertia_tensor * rb->torque;
                rb->angular_dp += platform_input->dt_per_frame * angular_ddp;
                // NOTE(gh) equation here is orientation += 0.5f*dt*angular_ddp*orientation
                rb->orientation += 0.5f*
                                   platform_input->dt_per_frame*
                                   Quat(0, rb->angular_dp) *
                                   rb->orientation;
                assert(is_pure_quat(rb->orientation));

                // NOTE(gh) clear computed parameters
                // TODO(gh) would be nice if we can put this in intialize rb function,
                entity->rb.force = V3();
                entity->rb.torque = V3();
            }break;
        }
    }

    RenderGroup render_group = {};
    start_render_group(&render_group, platform_render_push_buffer, &game_state->camera, V3(0, 0, 0));
#if 1
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
                // push_cube(&render_group, entity->aabb.p, 2.0f * entity->aabb.half_dim, V3(1, 1, 1), Quat(1, 0, 0, 0));
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
                // TODO(gh) collision volume independent rendering?
                push_cube(&render_group, entity->rb.p + entity->cv.offset, entity->cv.half_dim, entity->color, entity->rb.orientation);
            }break;
        }
    }
#endif

}

