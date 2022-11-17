/*
 * Written by Gyuhyun Lee
 */
#include "hb_types.h"
#include "hb_simd.h"
#include "hb_intrinsic.h"
#include "hb_math.h"
#include "hb_random.h"
#include "hb_font.h"
#include "hb_shared_with_shader.h"
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
        assert(platform_render_push_buffer->transient_buffer);

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

        v2 grid_dim = V2(80, 80); // TODO(gh) just temporary, need to 'gather' the floors later
        game_state->grass_grid_count_x = 2;
        game_state->grass_grid_count_y = 2;
        game_state->grass_grids = push_array(&game_state->transient_arena, GrassGrid, game_state->grass_grid_count_x*game_state->grass_grid_count_y);

        // TODO(gh) Beware that when you change this value, you also need to change the size of grass instance buffer
        // and the indirect command count(for now)
        u32 grass_per_grid_count_x = 256;
        u32 grass_per_grid_count_y = 256;

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
                    add_floor_entity(game_state, &game_state->transient_arena, center, grid_dim, V3(0.25f, 0.1f, 0.04f), grass_per_grid_count_x, grass_per_grid_count_y);

                GrassGrid *grid = game_state->grass_grids + y*game_state->grass_grid_count_x + x;
                init_grass_grid(thread_work_queue, gpu_work_queue, &game_state->transient_arena, floor_entity, grid, grass_per_grid_count_x, grass_per_grid_count_y, min, max);
            }
        }

        add_sphere_entity(game_state, &game_state->transient_arena, V3(0, 0, 2), 2.0f, V3(1, 0, 0));

        // TODO(gh) This means we have one vector per every 10m, which is not ideal.
        i32 fluid_cell_count_x = 24;
        i32 fluid_cell_count_y = 24;
        i32 fluid_cell_count_z = 8;
        f32 fluid_cell_dim = 12;
        v3 fluid_cell_left_bottom_p = V3(-fluid_cell_dim*fluid_cell_count_x/2, -fluid_cell_dim*fluid_cell_count_y/2, 0);
        // v3 fluid_cell_left_bottom_p = V3(0, 0, 0);

        initialize_fluid_cube_mac(&game_state->fluid_cube_mac, &game_state->transient_arena, gpu_work_queue,
                                    fluid_cell_left_bottom_p, V3i(fluid_cell_count_x, fluid_cell_count_y, fluid_cell_count_z), 
                                    fluid_cell_dim);

        load_game_assets(&game_state->assets, platform_api, gpu_work_queue);

        game_state->is_initialized = true;
    }

    Camera *game_camera = &game_state->game_camera;
    Camera *debug_camera = &game_state->debug_camera;

    Camera *render_camera = game_camera;
    //render_camera = debug_camera;
     
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

            case EntityType_Sphere:
            {
                platform_render_push_buffer->sphere_center = entity->p;
                platform_render_push_buffer->sphere_r = entity->dim.x;
            }break;
        }
    }

#if 0
    // NOTE(gh) Fluid simulation
    // TODO(gh) Do wee need to add source to the density, too?
    FluidCube *fluid_cube = &game_state->fluid_cube;
    
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

    // advect only works in zero-divergence field
    project(fluid_cube->v_x_dest, fluid_cube->v_y_dest, fluid_cube->v_z_dest, fluid_cube->pressures, fluid_cube->density_source, fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame);

#if 1
    reverse_advect(fluid_cube->v_x_dest, fluid_cube->v_x_source, fluid_cube->v_x_source, fluid_cube->v_y_source, fluid_cube->v_z_source, 
            fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame, ElementTypeForBoundary_NegateX);
    reverse_advect(fluid_cube->v_y_dest, fluid_cube->v_y_source, fluid_cube->v_x_source, fluid_cube->v_y_source, fluid_cube->v_z_source, 
            fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame, ElementTypeForBoundary_NegateY);
    reverse_advect(fluid_cube->v_z_dest, fluid_cube->v_z_source, fluid_cube->v_x_source, fluid_cube->v_y_source, fluid_cube->v_z_source, 
            fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame, ElementTypeForBoundary_NegateZ);
#endif

    // using density source as a temporary divergence buffer, which means modifying density source 
    // should come after this!
    project(fluid_cube->v_x_dest, fluid_cube->v_y_dest, fluid_cube->v_z_dest, fluid_cube->pressures, fluid_cube->density_source, fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame);
    // validate_divergence(fluid_cube->v_x_dest, fluid_cube->v_y_dest, fluid_cube->v_z_dest, fluid_cube->cell_count, fluid_cube->cell_dim);

    zero_memory(fluid_cube->density_source, fluid_cube->stride);
    local_persist b32 added_density = false;
    if(!added_density && platform_input->time_elasped_from_start > 1)
    {
        u32 ID = get_index(fluid_cube->cell_count, fluid_cube->cell_count.x/2, 1, fluid_cube->cell_count.z/2);
        fluid_cube->density_source[ID] = 10000;
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

    reverse_advect(fluid_cube->density_dest, fluid_cube->density_source, fluid_cube->v_x_dest, fluid_cube->v_y_dest, fluid_cube->v_z_dest, 
                    fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame, ElementTypeForBoundary_Continuous);
#if 0
    forward_advect(fluid_cube->density_dest, fluid_cube->density_source, fluid_cube->temp_buffer, fluid_cube->v_x_dest, fluid_cube->v_y_dest, fluid_cube->v_z_dest, 
                    fluid_cube->cell_count, fluid_cube->cell_dim, platform_input->dt_per_frame, ElementTypeForBoundary_Continuous);
#endif
#endif

    FluidCubeMAC *fluid_cube = &game_state->fluid_cube_mac;
    update_fluid_cube_mac(fluid_cube, &game_state->transient_arena, thread_work_queue, platform_input->dt_per_frame);
    // TODO(gh) Only a temporary thing, remove this when we move the whole fluid simluation to the GPU
    platform_render_push_buffer->fluid_cube_v_x = &fluid_cube->v_x;
    platform_render_push_buffer->fluid_cube_v_y = &fluid_cube->v_y;
    platform_render_push_buffer->fluid_cube_v_z = &fluid_cube->v_z;
    platform_render_push_buffer->fluid_cube_min = fluid_cube->min;
    platform_render_push_buffer->fluid_cube_max = fluid_cube->max;
    platform_render_push_buffer->fluid_cube_cell_count = fluid_cube->cell_count;
    platform_render_push_buffer->fluid_cube_cell_dim = fluid_cube->cell_dim;
    platform_render_push_buffer->fluid_cube_v_x_offset = clamp(0, (i32)((i64)fluid_cube->v_x_dest - (i64)fluid_cube->v_x_source), INT32_MAX);
    platform_render_push_buffer->fluid_cube_v_y_offset = clamp(0, (i32)((i64)fluid_cube->v_y_dest - (i64)fluid_cube->v_y_source), INT32_MAX);
    platform_render_push_buffer->fluid_cube_v_z_offset = clamp(0, (i32)((i64)fluid_cube->v_z_dest - (i64)fluid_cube->v_z_source), INT32_MAX);

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
                // TODO(gh) Don't pass mesh asset ID!!!
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
                // TODO(gh) Don't pass mesh asset ID!!!
                push_mesh_pn(platform_render_push_buffer, 
                          entity->p, entity->dim, entity->color, 
                          &game_state->assets, gpu_work_queue,  
                          entity->vertices, entity->vertex_count, entity->indices, entity->index_count,
                          &entity->mesh_assetID, true);
            }break;
        }
    }

    b32 enable_fluid_arrow_rendering = true;
    if(enable_fluid_arrow_rendering)
    {
        // NOTE(gh) Default arrow is looking down
        VertexPN arrow_vertices[] = 
        {
            {{V3(-0.5f, -0.5f, 0)}, {0, 0, 1}},
            {{V3(0.5f, -0.5f, 0)}, {0, 0, 1}},
            {{V3(-0.5f, 0.5f, 0)}, {0, 0, 1}},
            {{V3(0.5f, 0.5f, 0)}, {0, 0, 1}},

            {{V3(0, 0, -1)}, {0, 0, -1}},
        };

        u32 arrow_indices[] = 
        {
            0, 1, 2,
            1, 3, 2,
            0, 4, 1,
            1, 4, 3,
            3, 4, 2,
            2, 4, 0,
        };

        RenderEntryHeader *entry_header = 
            start_instanced_rendering(platform_render_push_buffer, 
                                        arrow_vertices, sizeof(arrow_vertices[0]), array_count(arrow_vertices),
                                        arrow_indices, sizeof(arrow_indices[0]), array_count(arrow_indices));

        for(u32 z = 0;
                z < fluid_cube->cell_count.z;
                ++z)
        {
            for(u32 y = 0;
                    y < fluid_cube->cell_count.y;
                    ++y)
            {
                for(u32 x = 0;
                        x < fluid_cube->cell_count.x;
                        ++x)
                {
                    v3 center = fluid_cube->min + fluid_cube->cell_dim*V3(x+0.5f, y+0.5f, z+0.5f);

                    f32 v_x = get_mac_center_value_x(fluid_cube->v_x_dest, x, y, z, fluid_cube->cell_count);
                    f32 v_y = get_mac_center_value_y(fluid_cube->v_y_dest, x, y, z, fluid_cube->cell_count);
                    f32 v_z = get_mac_center_value_z(fluid_cube->v_z_dest, x, y, z, fluid_cube->cell_count);

                    v3 color = {};

                    f32 scale = 0.5f;
                    v3 v = V3(v_x, v_y, v_z);
                    f32 v_scale = length(v)/3;
                    v = normalize(v);

                    if(compare_with_epsilon(length_square(v), 0))
                    {
                        scale = 0;
                    }
                    else
                    {
                        color = abs(v);
                    }

                    v3 rotation_axis = {};
                    f32 rotation_cos = 0;
                    if(compare_with_epsilon(v.x, 0) && compare_with_epsilon(v.y, 0))
                    {
                        rotation_axis = V3(1, 0, 0);

                        if(compare_with_epsilon(v.z, 1))
                        {
                            rotation_cos = -1;
                        }
                    }
                    else
                    {
                        rotation_axis = normalize(cross(V3(0, 0, -1), v));
                        // NOTE(gh) v is already normalized
                        rotation_cos = dot(V3(0, 0, -1), v);
                    }

                    f32 rotation_angle = acosf(rotation_cos)/2.0f;

                    push_render_entry_instance(platform_render_push_buffer, entry_header, center, V3(scale, scale, v_scale), rotation_axis, rotation_angle, color);
                    // push_arbitrary_mesh(platform_render_push_buffer, color, arrow_vertices, array_count(arrow_vertices), arrow_indices, array_count(arrow_indices));
                }
            }
        }

        end_instanced_rendering(platform_render_push_buffer);
    }

    b32 enable_ink_rendering = false;
    if(enable_ink_rendering)
    {
        VertexPN cube_vertices[] = 
        {
            {{0.5f*V3(-1, -1, -1)}, {1, 0, 0}},
            {{0.5f*V3(1, -1, -1)}, {1, 0, 0}},
            {{0.5f*V3(-1, 1, -1)}, {1, 0, 0}},
            {{0.5f*V3(1, 1, -1)}, {1, 0, 0}},

            {{0.5f*V3(-1, -1, 1)}, {1, 0, 0}},
            {{0.5f*V3(1, -1, 1)}, {1, 0, 0}},
            {{0.5f*V3(-1, 1, 1)}, {1, 0, 0}},
            {{0.5f*V3(1, 1, 1)}, {1, 0, 0}},
        };

        for(u32 i = 0;
                i < array_count(cube_vertices);
                ++i)
        {
            VertexPN *vertex = cube_vertices + i;
            vertex->n = normalize(vertex->p);
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

        RenderEntryHeader *entry_header = 
            start_instanced_rendering(platform_render_push_buffer, 
                    cube_vertices, sizeof(cube_vertices[0]), array_count(cube_vertices),
                    cube_indices, sizeof(cube_indices[0]), array_count(cube_indices));

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
                    v3 center = fluid_cube->min + fluid_cube->cell_dim*V3(cell_x+0.5f, cell_y+0.5f, cell_z+0.5f);
                    u32 ID = get_mac_index_center(cell_x, cell_y, cell_z, fluid_cube->cell_count);

                    f32 density = fluid_cube->density_dest[ID];
                    f32 scale = clamp(0.0f, density, fluid_cube->cell_dim);
                    // assert(density >= 0);

                    push_render_entry_instance(platform_render_push_buffer, entry_header, 
                                                center, scale*V3(1, 1, 1), V3(0, 0, 0), 0, V3(1, 1, 1));
                }
            }
        }

        end_instanced_rendering(platform_render_push_buffer);
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
#if 0
            debug_text_line(platform_render_push_buffer, font_asset, buffer, top_left_rel_p_px, scale);
            debug_newline(&top_left_rel_p_px, scale, &assets->debug_font_asset);
#endif

            atomic_exchange(&record->hit_count_cycle_count, 0);
        }
    }

#if 1 
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



