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
#include "meka_entity.cpp"
#include "meka_terrain.cpp"
#include "meka_render_group.cpp"
#include "meka_ray.cpp"
#include "meka_image_loader.cpp"
#include "meka_simulation.cpp"

internal void
update_and_render(PlatformAPI *platform_api, PlatformInput *platform_input, PlatformMemory *platform_memory,
                    u8 *render_push_buffer_base, u32 render_push_buffer_max_size, u32 *render_push_buffer_used)
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
#if 0
        game_state->voxel_arena = start_memory_arena(platform_memory->permanent_memory, megabytes(256));
        initialize_voxel_world(world);
        allocate_voxels_from_vox_file(world, &game_state->voxel_arena, loaded_vox);
#endif
#if 1
        Entity *entity = add_normalized_voxel_entity(game_state, V3(0, 0, 5), V3(1, 0, 0), 1.0f);
        entity = add_normalized_voxel_entity(game_state, V3(0, 0, 0), V3(0, 1, 0), 1.0f);
#endif
        //Entity *entity = add_normalized_voxel_entity(game_state, V3(0, 0, 0), V3(1, 0, 0));
        add_room_entity(game_state, V3(), V3(20, 20, 20));
        
        game_state->render_arena = start_memory_arena(platform_memory->transient_memory, megabytes(256));
        game_state->camera.focal_length = 1.0f;
        game_state->camera.p = V3(-10, 0, 0);
        game_state->camera.initial_z_axis = V3(-1, 0, 0);
        game_state->camera.initial_x_axis = normalize(cross(V3(0, 0, 1), game_state->camera.initial_z_axis));
        game_state->camera.initial_y_axis = normalize(cross(game_state->camera.initial_z_axis, game_state->camera.initial_x_axis));

        //game_state->voxel_position_buffer = mtl_create_managed_buffer(render_context, megabytes(4));
        //game_state->voxel_color_buffer = mtl_create_managed_buffer(render_context, megabytes(4));

        //RenderMesh cube_mesh = create_render_mesh(render_context, vertices);

        game_state->is_initialized = true;
    }

    Camera *camera= &game_state->camera;

    v3 camera_dir = camera_to_world(-camera->initial_z_axis, camera->initial_x_axis, camera->along_x, 
                                    camera->initial_y_axis, camera->along_y,
                                    camera->initial_z_axis, camera->along_z);

    r32 camera_speed = 10.0f * platform_input->dt_per_frame;

    if(platform_input->move_up)
    {
        camera->p += camera_speed*camera_dir;
    }

    if(platform_input->move_down)
    {
        camera->p -= camera_speed*camera_dir;
    }

#if 1
    r32 camera_rotation_speed = 0.5f * platform_input->dt_per_frame;
    if(platform_input->action_up)
    {
        camera->along_x += camera_rotation_speed;
    }
    if(platform_input->action_down)
    {
        camera->along_x -= camera_rotation_speed;
    }
    if(platform_input->action_left)
    {
        camera->along_y += camera_rotation_speed;
    }
    if(platform_input->action_right)
    {
        camera->along_y -= camera_rotation_speed;
    }
#endif

    b32 shoot_projectile = false;
    if(platform_input->space)
    {
        shoot_projectile = true;
    }
    
    // NOTE(joon) update entity start
    for(u32 entity_index = 0;
        entity_index < game_state->entity_count;
        ++entity_index)
    {
        Entity *entity = game_state->entities + entity_index;
        switch(entity->type)
        {
            case Entity_Type_Voxel:
            {
                move_entity(game_state, entity, platform_input->dt_per_frame);
            }
        }
    }

    // NOTE(joon) rendering code start
    RenderGroup render_group = {};
    start_render_group(&render_group, &game_state->camera, render_push_buffer_base, render_push_buffer_max_size);
    
    for(u32 entity_index = 0;
        entity_index < game_state->entity_count;
        ++entity_index)
    {
        Entity *entity = game_state->entities + entity_index;
        switch(entity->type)
        {
            case Entity_Type_Voxel:
            {
                push_voxel(&render_group, entity->p, entity->color);
            }break;
            case Entity_Type_Room:
            {
                push_room(&render_group, entity->p, entity->dim, entity->color);
            }break;
        }
    }

    // TODO(joon) don't exactly love this...
    *render_push_buffer_used = render_group.push_buffer_used;
}

