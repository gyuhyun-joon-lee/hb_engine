/*
 * Written by Gyuhyun Lee
 */
#include "hb_types.h"
#include "hb_simd.h"
#include "hb_intrinsic.h"
#include "hb_math.h"
#include "hb_random.h"
#include "hb_font.h"
#include "hb_render.h"
#include "hb_simulation.h"
#include "hb_entity.h"
#include "hb_fluid.h"
#include "hb_asset.h"
#include "hb_platform.h"
#include "hb_debug.h"
#include "hb.h"

#include "hb_ray.cpp"
#include "hb_simulation.cpp"
#include "hb_noise.cpp"
#include "hb_mesh_generation.cpp"
#include "hb_entity.cpp"
#include "hb_asset.cpp"
#include "hb_render.cpp"
#include "hb_image_loader.cpp"
#include "hb_fluid.cpp"

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

        game_state->game_camera = init_fps_camera(V3(0, -10, 22), 1.0f, 135, 1.0f, 1000.0f);
        game_state->game_camera.pitch += 1.5f;
        game_state->debug_camera = init_fps_camera(V3(0, 0, 22), 1.0f, 135, 0.1f, 10000.0f);
        // Close camera
        // game_state->circle_camera = init_circle_camera(V3(0, 0, 5), V3(0, 0, 0), 5.0f, 135, 0.01f, 10000.0f);
        // Far away camera
        game_state->circle_camera = init_circle_camera(V3(0, 0, 20), V3(0, 0, 0), 20.0f, 135, 0.01f, 10000.0f);
        // Really far away camera
        // game_state->circle_camera = init_circle_camera(V3(0, 0, 50), V3(0, 0, 0), 50.0f, 135, 0.01f, 10000.0f);

        v2 grid_dim = V2(100, 100); // TODO(gh) just temporary, need to 'gather' the floors later
        game_state->grass_grid_count_x = 4;
        game_state->grass_grid_count_y = 4;
        game_state->grass_grids = push_array(&game_state->transient_arena, GrassGrid, game_state->grass_grid_count_x*game_state->grass_grid_count_y);

        // TODO(gh) Beware that when you change this value, you also need to change the size of grass instance buffer
        // and the indirect command count(for now)
        u32 grass_on_floor_count_x = 256;
        u32 grass_on_floor_count_y = 256;

        v2 floor_left_bottom_p = hadamard(-V2(game_state->grass_grid_count_x/2, game_state->grass_grid_count_y/2), grid_dim);
        for(u32 y = 0;
                y < game_state->grass_grid_count_y;
                ++y)
        {
            for(u32 x = 0;
                    x < game_state->grass_grid_count_x;
                    ++x)
            {
                v2 min = floor_left_bottom_p + hadamard(grid_dim, V2(x, y));
                v2 max = min + grid_dim;
                v3 center = V3(0.5f*(min + max), 0);

                Entity *floor_entity = 
                    add_floor_entity(game_state, &game_state->transient_arena, center, grid_dim, V3(0.25f, 0.1f, 0.04f), grass_on_floor_count_x, grass_on_floor_count_y);

                GrassGrid *grid = game_state->grass_grids + y*game_state->grass_grid_count_x + x;
                init_grass_grid(platform_render_push_buffer, floor_entity, &game_state->random_series, grid, grass_on_floor_count_x, grass_on_floor_count_y, min, max);
            }
        }

        add_sphere_entity(game_state, &game_state->transient_arena, V3(0, 0, 2), 2.0f, V3(1, 0, 0));

        // TODO(gh) This means we have one vector per every 10m, which is not ideal.
        u32 fluid_cell_count_x = 32;
        u32 fluid_cell_count_y = 32;
        u32 fluid_cell_count_z = 16;
        initialize_fluid_cube(&game_state->fluid_cube, &game_state->transient_arena, 
                            V3(0, 0, 0), fluid_cell_count_x, fluid_cell_count_y, fluid_cell_count_z, 2, 3);

        load_game_assets(&game_state->assets, platform_api, gpu_work_queue, platform_render_push_buffer->device);
        game_state->debug_fluid_force_z = 1;

        game_state->is_initialized = true;
    }

    Camera *game_camera = &game_state->game_camera;
    Camera *debug_camera = &game_state->debug_camera;

    Camera *render_camera = game_camera;
    //render_camera = debug_camera;
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

    u32 update_perlin_noise_buffer_data_count = game_state->grass_grid_count_x * game_state->grass_grid_count_y;
    TempMemory perlin_noise_temp_memory = 
        start_temp_memory(&game_state->transient_arena, 
                sizeof(ThreadUpdatePerlinNoiseBufferData)*update_perlin_noise_buffer_data_count);
    ThreadUpdatePerlinNoiseBufferData *update_perlin_noise_buffer_data = 
        push_array(&perlin_noise_temp_memory, ThreadUpdatePerlinNoiseBufferData, update_perlin_noise_buffer_data_count);

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
        thread_work_queue->add_thread_work_queue_item(thread_work_queue, thread_update_perlin_noise_buffer_callback, 0, (void *)data);

        assert(update_perlin_noise_buffer_data_used_count <= update_perlin_noise_buffer_data_count);
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

            case EntityType_Sphere:
            {
                platform_render_push_buffer->sphere_center = entity->p;
                platform_render_push_buffer->sphere_r = entity->dim.x;
            }break;
        }
    }

    // NOTE(gh) Fluid simulation
    // TODO(gh) Do wee need to add source to the density, too?
    FluidCube *fluid_cube = &game_state->fluid_cube;
    
#if 0
    zero_memory(fluid_cube->v_x_source, fluid_cube->stride);
    zero_memory(fluid_cube->v_y_source, fluid_cube->stride);
    zero_memory(fluid_cube->v_z_source, fluid_cube->stride);
    //if(((u32)platform_input->time_elasped_from_start) % 4 == 0)
    u32 center_x = fluid_cube->cell_count.x/2;
    u32 center_y = fluid_cube->cell_count.y/2;

    for(i32 z = 1;
            z < fluid_cube->cell_count.z - 1;
            ++z)
    {
        for(i32 y = 1;
                y < 3;
                ++y)
       {
            for(i32 x = 1;
                    x < fluid_cube->cell_count.x - 1;
                    ++x)
            {
                u32 ID = get_index(fluid_cube->cell_count, x, y, z);

                fluid_cube->v_x_source[ID] = 0;
                fluid_cube->v_y_source[ID] = 30;
                fluid_cube->v_z_source[ID] = 30;
            }
        }
    }

    // NOTE(gh) At first, source holds the source force, and dest holds the previous value
    // adding force will overwrite the new value to dest 

    // velocity
    add_source(fluid_cube->v_x_dest, fluid_cube->v_x_dest, fluid_cube->v_x_source, fluid_cube->cell_count, platform_input->dt_per_frame);
    add_source(fluid_cube->v_y_dest, fluid_cube->v_y_dest, fluid_cube->v_y_source, fluid_cube->cell_count, platform_input->dt_per_frame);
    add_source(fluid_cube->v_z_dest, fluid_cube->v_z_dest, fluid_cube->v_z_source, fluid_cube->cell_count, platform_input->dt_per_frame);

    swap(fluid_cube->v_x_dest, fluid_cube->v_x_source);
    swap(fluid_cube->v_y_dest, fluid_cube->v_y_source);
    swap(fluid_cube->v_z_dest, fluid_cube->v_z_source);

    u32 diffuse_iter_count = 1;
    for(u32 diffuse_iter = 0;
            diffuse_iter < diffuse_iter_count;
            ++diffuse_iter)
    {
        diffuse(fluid_cube->v_x_dest, fluid_cube->v_x_source, fluid_cube->cell_count, fluid_cube->cell_dim, 
                fluid_cube->viscosity, platform_input->dt_per_frame, ElementTypeForBoundary_NegateX);
        diffuse(fluid_cube->v_y_dest, fluid_cube->v_y_source, fluid_cube->cell_count, fluid_cube->cell_dim, 
                fluid_cube->viscosity, platform_input->dt_per_frame, ElementTypeForBoundary_NegateY);
        diffuse(fluid_cube->v_z_dest, fluid_cube->v_z_source, fluid_cube->cell_count, fluid_cube->cell_dim, 
                fluid_cube->viscosity, platform_input->dt_per_frame, ElementTypeForBoundary_NegateZ);

        swap(fluid_cube->v_x_dest, fluid_cube->v_x_source);
        swap(fluid_cube->v_y_dest, fluid_cube->v_y_source);
        swap(fluid_cube->v_z_dest, fluid_cube->v_z_source);
    }

    advect(fluid_cube->v_x_dest, fluid_cube->v_x_source, fluid_cube->v_x_source, fluid_cube->v_y_source, fluid_cube->v_z_source, 
            fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame, ElementTypeForBoundary_NegateX);
    advect(fluid_cube->v_y_dest, fluid_cube->v_y_source, fluid_cube->v_x_source, fluid_cube->v_y_source, fluid_cube->v_z_source, 
            fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame, ElementTypeForBoundary_NegateY);
    advect(fluid_cube->v_z_dest, fluid_cube->v_z_source, fluid_cube->v_x_source, fluid_cube->v_y_source, fluid_cube->v_z_source, 
            fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame, ElementTypeForBoundary_NegateZ);

    // using density source as a temporary divergence buffer, which means modifying divergence source buffer
    // should come after this!
    project(fluid_cube->v_x_dest, fluid_cube->v_y_dest, fluid_cube->v_z_dest, fluid_cube->pressures, fluid_cube->density_source, fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame);
    // validate_divergence(fluid_cube->v_x_dest, fluid_cube->v_y_dest, fluid_cube->v_z_dest, fluid_cube->cell_count, fluid_cube->cell_dim);

    zero_memory(fluid_cube->density_source, fluid_cube->stride);
    local_persist b32 added_density = false;
    if(!added_density && platform_input->time_elasped_from_start > 8)
    {
        fluid_cube->density_source[get_index(fluid_cube->cell_count, fluid_cube->cell_count.x/2, 1, fluid_cube->cell_count.z/2)] = 
            10000;
        added_density = true;
    }
    
#if 0
    printf("%.6f, %.6f, %.6f\n", fluid_cube->v_x_dest[get_index(fluid_cube->cell_count, 1, 2, 1)],
                                 fluid_cube->v_y_dest[get_index(fluid_cube->cell_count, 1, 2, 1)],
                                 fluid_cube->v_z_dest[get_index(fluid_cube->cell_count, 1, 2, 1)]);
#endif

    // density
    add_source(fluid_cube->density_dest, fluid_cube->density_dest, fluid_cube->density_source, fluid_cube->cell_count, platform_input->dt_per_frame);

    swap(fluid_cube->density_dest, fluid_cube->density_source);

    for(u32 diffuse_iter = 0;
            diffuse_iter < diffuse_iter_count;
            ++diffuse_iter)
    {
        diffuse(fluid_cube->density_dest, fluid_cube->density_source, fluid_cube->cell_count, fluid_cube->cell_dim, 
                fluid_cube->viscosity, platform_input->dt_per_frame, ElementTypeForBoundary_Continuous);
        swap(fluid_cube->density_dest, fluid_cube->density_source);
    }
    
    advect(fluid_cube->density_dest, fluid_cube->density_source, fluid_cube->v_x_dest, fluid_cube->v_y_dest, fluid_cube->v_z_dest, 
            fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame, ElementTypeForBoundary_Continuous);
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

    if(debug_platform_render_push_buffer)
    {
        init_render_push_buffer(debug_platform_render_push_buffer, render_camera, game_camera,
                    0, 0, 0, V3(0, 0, 0), false);
    }

    for(u32 entity_index = 0;
        entity_index < game_state->entity_count;
        ++entity_index)
    {
        Entity *entity = game_state->entities + entity_index;
        switch(entity->type)
        {
            case EntityType_Floor:
            {
                // TODO(gh) Don't pass platform api, device, mesh asset ID!!!
                push_mesh_pn(platform_render_push_buffer, 
                          entity->p, entity->dim, entity->color, 
                          &game_state->assets, gpu_work_queue,  
                          entity->vertices, entity->vertex_count, entity->indices, entity->index_count,
                          &entity->mesh_assetID, false);
            }break;
            case EntityType_AABB:
            case EntityType_Cube:
            case EntityType_Sphere:
            {
                // TODO(gh) Don't pass platform api, device, mesh asset ID!!!
                push_mesh_pn(platform_render_push_buffer, 
                          entity->p, entity->dim, entity->color, 
                          &game_state->assets, gpu_work_queue,  
                          entity->vertices, entity->vertex_count, entity->indices, entity->index_count,
                          &entity->mesh_assetID, true);
            }break;
        }
    }
    b32 enable_fluid_arrow_rendering = false;

    if(enable_fluid_arrow_rendering)
    {
        for(u32 cell_z = 0;
                cell_z < fluid_cube->cell_count.z;
                ++cell_z)
        {
            for(u32 cell_y = 0;
                    cell_y < fluid_cube->cell_count.y;
                    ++cell_y)
            {
                for(u32 cell_x = 0;
                        cell_x < fluid_cube->cell_count.x;
                        ++cell_x)
                {
                    v3 center = fluid_cube->left_bottom_p + fluid_cube->cell_dim*V3(cell_x+0.5f, cell_y+0.5f, cell_z+0.5f);

                    f32 v_x = fluid_cube->v_x_dest[get_index(fluid_cube->cell_count, cell_x, cell_y, cell_z)];
                    f32 v_y = fluid_cube->v_y_dest[get_index(fluid_cube->cell_count, cell_x, cell_y, cell_z)];
                    f32 v_z = fluid_cube->v_z_dest[get_index(fluid_cube->cell_count, cell_x, cell_y, cell_z)];

                    v3 color = V3(0.5f, 0.5f, 0.5f);

                    f32 scale = 0.15f;
                    v3 v = V3(v_x, v_y, v_z);
                    f32 v_scale = clamp(0.0f, length(v), 0.5f*fluid_cube->cell_dim);
                    v = normalize(v);

                    if(compare_with_epsilon(length_square(v), 0))
                    {
                        scale = 0;
                    }
                    else
                    {
                        color = abs(v);
                    }

                    if(cell_x == 0 || (cell_x == fluid_cube->cell_count.x-1) || 
                            cell_y == 0 || (cell_y == fluid_cube->cell_count.y-1) ||
                            cell_z == 0 || (cell_z == fluid_cube->cell_count.z-1))
                    {
                        color = V3(1, 0, 0);
                    }

                    // TODO(gh) Doesn't work when v == up vector, and what if the length of v is too small?
                    v3 right = V3();
                    if(v.x == 0 && v.y == 0 && v.z != 0.0f)
                    {
                        right = scale*normalize(cross(v, V3(1, 0, 0)));
                    }
                    else
                    {
                        right = scale*normalize(cross(v, V3(0, 0, 1)));
                    }

                    v3 up = scale*normalize(cross(right, v));
                    VertexPN arrow_vertices[] = 
                    {
                        {{center - right - up}, {1, 0, 0}},
                        {{center + right - up}, {1, 0, 0}},
                        {{center - right + up}, {1, 0, 0}},
                        {{center + right + up},{1, 0, 0}},
                        {{center + v_scale*v}, {1, 0, 0}},
                    };

                    u32 arrow_indices[] = 
                    {
                        0, 3, 2,
                        0, 1, 3,
                        2, 3, 4, 
                        3, 1, 4, 
                        0, 4, 1, 
                        0, 2, 4, 
                    };

                    push_arbitrary_mesh(platform_render_push_buffer, color, arrow_vertices, array_count(arrow_vertices), arrow_indices, array_count(arrow_indices));
                }
            }
        }
    }

    b32 enable_ink_rendering = false;
    if(enable_ink_rendering)
    {
        for(u32 cell_z = 0;
                cell_z < fluid_cube->cell_count.z;
                ++cell_z)
        {
            for(u32 cell_y = 0;
                    cell_y < fluid_cube->cell_count.y;
                    ++cell_y)
            {
                for(u32 cell_x = 0;
                        cell_x < fluid_cube->cell_count.x;
                        ++cell_x)
                {
                    v3 center = fluid_cube->left_bottom_p + fluid_cube->cell_dim*V3(cell_x+0.5f, cell_y+0.5f, cell_z+0.5f);
                    u32 ID = get_index(fluid_cube->cell_count, cell_x, cell_y, cell_z);

                    f32 density = fluid_cube->density_dest[ID];
                    assert(density >= 0);

                    f32 scale = clamp(0.0f, fluid_cube->cell_dim*density, 2.0f);
                    {
                        VertexPN cube_vertices[] = 
                        {
                            {{center + 0.5f*scale*V3(-1, -1, -1)}, {1, 0, 0}},
                            {{center + 0.5f*scale*V3(1, -1, -1)}, {1, 0, 0}},
                            {{center + 0.5f*scale*V3(-1, 1, -1)}, {1, 0, 0}},
                            {{center + 0.5f*scale*V3(1, 1, -1)}, {1, 0, 0}},

                            {{center + 0.5f*scale*V3(-1, -1, 1)}, {1, 0, 0}},
                            {{center + 0.5f*scale*V3(1, -1, 1)}, {1, 0, 0}},
                            {{center + 0.5f*scale*V3(-1, 1, 1)}, {1, 0, 0}},
                            {{center + 0.5f*scale*V3(1, 1, 1)}, {1, 0, 0}},
                        };

                        for(u32 i = 0;
                                i < array_count(cube_vertices);
                                ++i)
                        {
                            VertexPN *vertex = cube_vertices + i;
                            vertex->n = normalize(vertex->p - center);
                        }

                        u32 cube_indices[] = 
                        {
                            0, 1, 4, 1, 5, 4,
                            1, 3, 5, 3, 7, 5,
                            6, 7, 3, 6, 3, 2,
                            4, 2, 0, 4, 6, 2,
                            7, 6, 5, 5, 6, 4,
                            2, 1, 0, 2, 3, 1,
                        };

                        push_arbitrary_mesh(platform_render_push_buffer, V3(1, 1, 1), cube_vertices, array_count(cube_vertices), cube_indices, array_count(cube_indices));
                    }
                }
            }
        }
    }


    if(debug_platform_render_push_buffer)
    {
        // NOTE(gh) push forward render entries
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

            push_frustum(debug_platform_render_push_buffer, V3(0, 0.2f, 1), 
                    frustum_vertices, array_count(frustum_vertices),
                    frustum_indices, array_count(frustum_indices));

            v3 line_color = V3(0, 1, 1);

            push_line(debug_platform_render_push_buffer, game_camera_frustum.near[0], game_camera_frustum.near[1], line_color);
            push_line(debug_platform_render_push_buffer, game_camera_frustum.near[1], game_camera_frustum.near[3], line_color);
            push_line(debug_platform_render_push_buffer, game_camera_frustum.near[2], game_camera_frustum.near[3], line_color);
            push_line(debug_platform_render_push_buffer, game_camera_frustum.near[2], game_camera_frustum.near[0], line_color);

            push_line(debug_platform_render_push_buffer, game_camera_frustum.far[0], game_camera_frustum.far[1], line_color);
            push_line(debug_platform_render_push_buffer, game_camera_frustum.far[1], game_camera_frustum.far[3], line_color);
            push_line(debug_platform_render_push_buffer, game_camera_frustum.far[3], game_camera_frustum.far[2], line_color);
            push_line(debug_platform_render_push_buffer, game_camera_frustum.far[2], game_camera_frustum.far[0], line_color);

            push_line(debug_platform_render_push_buffer, game_camera_frustum.near[0], game_camera_frustum.far[0], line_color);
            push_line(debug_platform_render_push_buffer, game_camera_frustum.near[1], game_camera_frustum.far[1], line_color);
            push_line(debug_platform_render_push_buffer, game_camera_frustum.near[2], game_camera_frustum.far[2], line_color);
            push_line(debug_platform_render_push_buffer, game_camera_frustum.near[3], game_camera_frustum.far[3], line_color);
        }

        // TODO(gh) This prevents us from timing the game update and render loop itself 
        output_debug_records(debug_platform_render_push_buffer, &game_state->assets, V2(0, 0));
    }
    
    thread_work_queue->complete_all_thread_work_queue_items(thread_work_queue, true);

    end_temp_memory(&perlin_noise_temp_memory);

    // TODO(gh) simplify variables that can effect the wind 
    // maybe with world unit speed
    game_state->time_until_offset_x_inc += platform_input->dt_per_frame;
    if(game_state->time_until_offset_x_inc > 0.02f)
    {
        game_state->offset_x++;
        game_state->time_until_offset_x_inc = 0;
    }
}

// TODO(gh) we can use this function to make the p relative to bottom_left!
internal void
debug_reset_text_to_top_left(v2 *p)
{
    *p = V2(0, 0);
}

internal void
debug_text_line(PlatformRenderPushBuffer *platform_render_push_buffer, FontAsset *font_asset, const char *text, v2 top_left_rel_p_px, f32 scale)
{
    const char *c=text;
    while(*c != '\0')
    {
        if(*c != ' ')
        {
            // The texture that we got does not have bearing integrated,
            // which means we have to offset the entry based on the left bearing 
            f32 left_bearing_px = get_glyph_left_bearing_px(font_asset, scale, *c);
            push_glyph(platform_render_push_buffer, font_asset, 
                    V3(1, 1, 1), top_left_rel_p_px + V2(left_bearing_px, 0), *c, scale);
        }

        // NOTE(gh) check out https://freetype.org/freetype2/docs/tutorial/step2.html
        // for the terms

        f32 x_advance_px = get_glyph_x_advance_px(font_asset, scale, *c);
        if(*(c+1) != '\0')
        {
            f32 kerning = get_glyph_kerning(font_asset, scale, *(c), *(c+1));
            x_advance_px += (kerning);
        }

        top_left_rel_p_px.x += x_advance_px;
        c++;
    }
}

internal void
debug_newline(v2 *top_down_p, f32 scale, FontAsset *font_asset)
{
    top_down_p->x = 0;
    // TODO(gh) I don't like the fact that this function has to 'know' what font asset it is using..
    top_down_p->y += scale*(font_asset->ascent_from_baseline + 
                            font_asset->descent_from_baseline + 
                            font_asset->line_gap);
}

// NOTE(gh) This counter value will be inserted at at the end of the compilation, meaning this array will be large enough
// to contain all the records that we time_blocked in other codes
DebugRecord game_debug_records[__COUNTER__];

#include <stdio.h>
internal void
output_debug_records(PlatformRenderPushBuffer *platform_render_push_buffer, GameAssets *assets, v2 top_left_rel_p_px)
{
    FontAsset *font_asset = &assets->debug_font_asset;
    {
        f32 scale = 1.5f;
        // This does not take account of kerning, but
        // kanji might not have kerning at all
        push_glyph(platform_render_push_buffer, font_asset, 
                    V3(1, 1, 1), top_left_rel_p_px, 0x8349, scale);
        top_left_rel_p_px.x += get_glyph_x_advance_px(font_asset, scale, 0x8349);

        top_left_rel_p_px.x += get_glyph_x_advance_px(font_asset, scale, ' ');

        push_glyph(platform_render_push_buffer, font_asset, 
                    V3(1, 1, 1), top_left_rel_p_px, 0x30a8, scale);
        top_left_rel_p_px.x += get_glyph_x_advance_px(font_asset, scale, 0x8349);

        push_glyph(platform_render_push_buffer, font_asset, 
                    V3(1, 1, 1), top_left_rel_p_px, 0x30f3, scale);
        top_left_rel_p_px.x += get_glyph_x_advance_px(font_asset, scale, 0x8349);

        push_glyph(platform_render_push_buffer, font_asset, 
                    V3(1, 1, 1), top_left_rel_p_px, 0x30b8, scale);
        top_left_rel_p_px.x += get_glyph_x_advance_px(font_asset, scale, 0x8349);

        push_glyph(platform_render_push_buffer, font_asset, 
                    V3(1, 1, 1), top_left_rel_p_px, 0x30f3, scale);
        top_left_rel_p_px.x += get_glyph_x_advance_px(font_asset, scale, 0x8349);

        debug_newline(&top_left_rel_p_px, scale, font_asset);
    }
#if HB_DEBUG
    u64 total_cycle_count = 0;
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

            total_cycle_count += cycle_count;

            char buffer[512] = {};
            snprintf(buffer, array_count(buffer),
                    "%s(%s(%u)): %ucy, %uh, %ucy/h ", function, file, line, cycle_count, hit_count, cycle_count/hit_count);

            // TODO(gh) Do we wanna keep this scale value?
            f32 scale = 0.5f;
            // debug_text_line(platform_render_push_buffer, font_asset, buffer, top_left_rel_p_px, scale);
            //debug_text_line(platform_render_push_buffer, assets, "Togplil", top_left_rel_p_px, scale);

            debug_newline(&top_left_rel_p_px, scale, &assets->debug_font_asset);

            atomic_exchange(&record->hit_count_cycle_count, 0);
        }
    }

#if 0 
    {
        char buffer[512] = {};
        snprintf(buffer, array_count(buffer),
                "total cycle count : %llu", total_cycle_count);
        // TODO(gh) Do we wanna keep this scale value?
        f32 scale = 0.5f;
        debug_text_line(platform_render_push_buffer, font_asset, buffer, top_left_rel_p_px, scale);
    }
#endif
#endif

}



