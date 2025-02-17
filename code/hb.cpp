/*
 * Written by Gyuhyun Lee
 */
#include "hb_types.h"
#include "hb_simd.h"
#include "hb_intrinsic.h"
#include "hb_math.h"
#include "hb_matrix.h"
#include "hb_random.h"
#include "hb_font.h"
#include "hb_shared_with_shader.h"
#include "hb_render.h"
#include "hb_pbd.h"
#include "hb_entity.h"
#include "hb_fluid.h"
#include "hb_asset.h"
#include "hb_platform.h"
#include "hb_debug.h"
#include "hb_vox.h"
#include "hb.h"

#include "hb_ray.cpp"
#include "hb_noise.cpp"
#include "hb_mesh_generation.cpp"
#include "hb_pbd.cpp"
#include "hb_entity.cpp"
#include "hb_rigidbody.cpp"
#include "hb_asset.cpp"
#include "hb_render.cpp"
#include "hb_image_loader.cpp"
#include "hb_fluid.cpp"
#include "hb_obj.cpp"
#include "hb_vox.cpp"

// TODO(gh) Remove this dependency
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
    TranState *tran_state = (TranState *)platform_memory->transient_memory;
    if(!tran_state->is_initialized)
    {
        // NOTE(gh) Should start AFTER the tran state!!
        tran_state->transient_arena = start_memory_arena((u8 *)platform_memory->transient_memory + sizeof(TranState), 
                                                        gigabytes(1));

        {
        PlatformReadFileResult vox_file = platform_api->read_file("../data/3x3x3.vox");
        tran_state->loaded_voxs[tran_state->loaded_vox_count++] = load_vox(vox_file.memory, vox_file.size);
        }

        {
        PlatformReadFileResult vox_file = platform_api->read_file("../data/4x4x4.vox");
        tran_state->loaded_voxs[tran_state->loaded_vox_count++] = load_vox(vox_file.memory, vox_file.size);
        }

        {
        PlatformReadFileResult vox_file = platform_api->read_file("../data/5x5x5.vox");
        tran_state->loaded_voxs[tran_state->loaded_vox_count++] = load_vox(vox_file.memory, vox_file.size);
        }

        {
        PlatformReadFileResult vox_file = platform_api->read_file("../data/6x6x6.vox");
        tran_state->loaded_voxs[tran_state->loaded_vox_count++] = load_vox(vox_file.memory, vox_file.size);
        }

        {
        PlatformReadFileResult vox_file = platform_api->read_file("../data/8x8x8.vox");
        tran_state->loaded_voxs[tran_state->loaded_vox_count++] = load_vox(vox_file.memory, vox_file.size);
        }

        tran_state->game_camera = init_fps_camera(V3(0, -10, 22), 1.0f, 135, 1.0f, 1000.0f);
        tran_state->debug_camera = init_fps_camera(V3(0, 0, 22), 1.0f, 135, 0.1f, 10000.0f);


        // 30 seconds worth of frames
        tran_state->max_saved_game_state_count = round_f32_to_u32(1.0f/platform_input->dt_per_frame) * 15;
        tran_state->saved_game_states = push_array(&tran_state->transient_arena, GameState, tran_state->max_saved_game_state_count);
        tran_state->saved_game_state_read_cursor = 0;
        tran_state->saved_game_state_write_cursor = 0;
        tran_state->has_entire_buffer_filled_at_least_once = false;

        tran_state->max_pbd_substep_count = 16;
        tran_state->remaining_pbd_substep_count = tran_state->max_pbd_substep_count*50*60;
        tran_state->is_simulating_in_realtime = true;

        load_game_assets(&tran_state->assets, &tran_state->transient_arena, platform_api, gpu_work_queue);

        tran_state->is_initialized = true;
    }

    GameState *game_state = (GameState *)platform_memory->permanent_memory;

    if(!game_state->is_initialized)
    {
        assert(platform_render_push_buffer->transient_buffer);

#if 0
        v2 grid_dim = V2(80, 80); // TODO(gh) just temporary, need to 'gather' the floors later
        game_state->grass_grid_count_x = 2;
        game_state->grass_grid_count_y = 2;
        game_state->grass_grids = push_array(&game_state->transient_arena, GrassGrid, game_state->grass_grid_count_x*game_state->grass_grid_count_y);

        // TODO(gh) Beware that when you change this value, you also need to change the size of grass instance buffer
        // and the indirect command count(for now)
        u32 grass_per_grid_count_x = 256;
        u32 grass_per_grid_count_y = 256;

        // u32 grass_per_grid_count_x = 64;
        // u32 grass_per_grid_count_y = 64;

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
                    add_floor_entity(game_state, &game_state->transient_arena, center, grid_dim, V3(0.25f, 0.1f, 0.04f), grass_per_grid_count_x, grass_per_grid_count_y, 16);

                GrassGrid *grid = game_state->grass_grids + y*game_state->grass_grid_count_x + x;
                init_grass_grid(thread_work_queue, gpu_work_queue, &game_state->transient_arena, floor_entity, grid, grass_per_grid_count_x, grass_per_grid_count_y, min, max);
            }
        }
#endif
        game_state->random_series = start_random_series(12312312);

        add_floor_entity(game_state, V3(), V2(1000, 1000), V3(1.0f, 1.0f, 1.0f), 1, 1, 0);

#if 0
        {
            v3 color = V3(random_between_0_1(&game_state->random_series), 
                           random_between_0_1(&game_state->random_series),
                           random_between_0_1(&game_state->random_series));
            add_pbd_vox_entity(game_state, 
                             tran_state->loaded_voxs + 0,
                            V3d(1, 1, 3), V3d(0, 0, 0),
                            0.5f, 1.0f/(random_between(&game_state->random_series, 100, 300)), color, 
                            EntityFlag_Movable|EntityFlag_Collides|EntityFlag_RigidBody);
        }
#endif
#if 0
        {
            v3 color = V3(random_between_0_1(&game_state->random_series), 
                           random_between_0_1(&game_state->random_series),
                           random_between_0_1(&game_state->random_series));
            add_pbd_vox_entity(game_state,  
                             tran_state->loaded_voxs + 1,
                            V3d(-9, -9, 20), V3d(0, 0, 0),
                            0.1f, 1.0f/(random_between(&game_state->random_series, 5, 40)), color, 
                            EntityFlag_Movable|EntityFlag_Collides|EntityFlag_RigidBody);
        }
#endif
#if 1
        {


            v3 color = V3(random_between_0_1(&game_state->random_series), 
                           random_between_0_1(&game_state->random_series),
                           random_between_0_1(&game_state->random_series));
            add_pbd_vox_entity(game_state, 
                             tran_state->loaded_voxs + 2,
                            V3d(3, 3, 12), V3d(0, 0, 0),
                            0.8f, 1.0f/(random_between(&game_state->random_series, 10, 50)), color, 
                            EntityFlag_Movable|EntityFlag_Collides|EntityFlag_Linear);
        }
#endif
#if 0
        {
            v3 color = V3(random_between_0_1(&game_state->random_series), 
                           random_between_0_1(&game_state->random_series),
                           random_between_0_1(&game_state->random_series));
            add_pbd_vox_entity(game_state,  
                             tran_state->loaded_voxs + 3,
                            V3d(7, 7, 5), V3d(0, 0, 0),
                            0.1f, 1.0f/(random_between(&game_state->random_series, 10, 50)), color, 
                            EntityFlag_Movable|EntityFlag_Collides|EntityFlag_Linear);
        }
#endif
#if 0
        {
            v3 color = V3(random_between_0_1(&game_state->random_series), 
                           random_between_0_1(&game_state->random_series),
                           random_between_0_1(&game_state->random_series));
            add_pbd_vox_entity(game_state, 
                             tran_state->loaded_voxs + 4,
                            V3d(-15, -15, 5), V3d(0, 0, 0),
                            0.3f, 1.0f/(random_between(&game_state->random_series, 80, 100)), color, 
                            EntityFlag_Movable|EntityFlag_Collides|EntityFlag_Linear);
        }
#endif


#if 0
        // TODO(gh) This means we have one vector per every 10m, which is not ideal.
        i32 fluid_cell_count_x = 16;
        i32 fluid_cell_count_y = 16;
        i32 fluid_cell_count_z = 16;
        f32 fluid_cell_dim = 12;
        v3 fluid_cell_left_bottom_p = V3(-fluid_cell_dim*fluid_cell_count_x/2, -fluid_cell_dim*fluid_cell_count_y/2, 0);


        initialize_fluid_cube_mac(&game_state->fluid_cube_mac, &game_state->transient_arena, gpu_work_queue,
                                    fluid_cell_left_bottom_p, V3i(fluid_cell_count_x, fluid_cell_count_y, fluid_cell_count_z), 
                                    fluid_cell_dim);
#endif

        game_state->is_initialized = true;
    }

    u64 game_state_size = sizeof(*game_state);
    u64 tran_state_size = sizeof(*tran_state);
    u64 entity_size = sizeof(game_state->entities);
    u64 particle_pool_size = sizeof(game_state->particle_pool);

    Camera *game_camera = &tran_state->game_camera;
    Camera *debug_camera = &tran_state->debug_camera;

    Camera *render_camera = game_camera;
    //render_camera = debug_camera;

    // This is an axis angle rotation, which will be a pure quaternion 
    // when used to update the orientation.
    v3 camera_rotation = V3();
     
    if(render_camera == debug_camera)
    {
        // game_camera->roll += 0.8f * platform_input->dt_per_frame;
    }

    v3 camera_dir = get_camera_lookat(render_camera);
    v3 camera_right = get_camera_right(render_camera);
    v3 camera_up = normalize(cross(camera_right, camera_dir)); 

    f32 camera_rotation_speed = 3.0f * platform_input->dt_per_frame;
    if(is_key_down(platform_input, PlatformKeyID_ActionUp))
    {
        camera_rotation += camera_rotation_speed * camera_right;
    }
    if(is_key_down(platform_input, PlatformKeyID_ActionDown))
    {
        camera_rotation -= camera_rotation_speed * camera_right;
    }
    if(is_key_down(platform_input, PlatformKeyID_ActionLeft))
    {
        camera_rotation += camera_rotation_speed * V3(0, 0, 1);
    }
    if(is_key_down(platform_input, PlatformKeyID_ActionRight))
    {
        camera_rotation -= camera_rotation_speed * V3(0, 0, 1);
    }

    // NOTE(gh) orientation, which is a unit quaternion, has 4 DOF, but we have to have only 3.
    // By normalizing the orientation, we can get 4-1=3DOF.
    // 0.5 comes from the quaternion differentiation
    render_camera->orientation = 
        normalize(render_camera->orientation +
                  0.5f*Quat(0, camera_rotation)*render_camera->orientation);

#if 1
    f32 camera_speed = 15.0f * platform_input->dt_per_frame;
    if(is_key_down(platform_input, PlatformKeyID_MoveUp))
    {
        render_camera->p += camera_speed*camera_dir;
    }
    if(is_key_down(platform_input, PlatformKeyID_MoveDown))
    {
        render_camera->p -= camera_speed*camera_dir;
    }
    if(is_key_down(platform_input, PlatformKeyID_MoveRight))
    {
        render_camera->p += camera_speed*camera_right;
    }
    if(is_key_down(platform_input, PlatformKeyID_MoveLeft))
    {
        render_camera->p += -camera_speed*camera_right;
    }
#endif

    // TODO(gh) It's really hard to keep track of which code should run or not
    // depending on whether the game is being simulated or not...
    if(tran_state->is_simulating_in_realtime)
    {
        tran_state->saved_game_states[tran_state->saved_game_state_write_cursor++] = *game_state;
        if(tran_state->saved_game_state_write_cursor >= tran_state->max_saved_game_state_count)
        {
            tran_state->saved_game_state_write_cursor = 0;
            tran_state->has_entire_buffer_filled_at_least_once = true;
        }
        tran_state->remaining_pbd_substep_count = tran_state->max_pbd_substep_count;

        if(is_key_pressed(platform_input, PlatformKeyID_Shoot))
        {
            add_pbd_single_particle_entity(game_state, 
                                            V3d(render_camera->p), V3d(50*camera_dir), 
                                            0.0f, 1.0f/10.0f, V3(0, 1, 0), EntityFlag_Movable|EntityFlag_Collides);
        }

        if(is_key_pressed(platform_input, PlatformKeyID_ToggleSimulation))
        {
            // Pause the simulation

            tran_state->saved_game_state_read_cursor = tran_state->saved_game_state_write_cursor - 1;
            tran_state->remaining_pbd_substep_count = 0;
            tran_state->is_simulating_in_realtime = false;
        }
    }
    else // not simulating
    {
#if HB_SLOW
        if(is_key_pressed(platform_input, PlatformKeyID_ToggleSimulation))
        {
            // Resume the simulation

            // TODO(gh) This will get set in next frame anyway, so might be an overkill
            // to initialize it here once more
            tran_state->remaining_pbd_substep_count = tran_state->max_pbd_substep_count;
            // NOTE(gh) We reset the write & read cursor, so that no weird time overlapping happens 
            tran_state->saved_game_state_write_cursor = 0;
            tran_state->saved_game_state_read_cursor = 0;
            tran_state->has_entire_buffer_filled_at_least_once = false;
            tran_state->is_simulating_in_realtime = true;
        }

        if(is_key_down(platform_input, PlatformKeyID_FallbackFrame))
        {
            if(tran_state->has_entire_buffer_filled_at_least_once)
            {
                if(tran_state->saved_game_state_read_cursor >= tran_state->max_saved_game_state_count)
                {
                    tran_state->saved_game_state_read_cursor = tran_state->max_saved_game_state_count-1;
                }
            }
            else
            {
                if(tran_state->saved_game_state_read_cursor >= tran_state->saved_game_state_write_cursor)
                {
                    tran_state->saved_game_state_read_cursor = tran_state->saved_game_state_write_cursor-1;
                }
            }

            *game_state = tran_state->saved_game_states[tran_state->saved_game_state_read_cursor--];
        }

        if(is_key_down(platform_input, PlatformKeyID_AdvanceFrame))
        {
            if(tran_state->has_entire_buffer_filled_at_least_once)
            {
                if(tran_state->saved_game_state_read_cursor >= tran_state->max_saved_game_state_count)
                {
                    tran_state->saved_game_state_read_cursor = 0;
                }
            }
            else
            {
                if(tran_state->saved_game_state_read_cursor >= tran_state->saved_game_state_write_cursor)
                {
                    tran_state->saved_game_state_read_cursor = 0;
                }
            }

            *game_state = tran_state->saved_game_states[tran_state->saved_game_state_read_cursor++];
        }

        if(is_key_down(platform_input, PlatformKeyID_AdvanceSubstep))
        {
            tran_state->remaining_pbd_substep_count = 1;
        }
    }
#endif

    /*
       NOTE(gh) A little bit about how PBD works
       Currently we are using Gauss - Seidel relaxation, which means for every iteration,
       we are using the updated position to get the new position. However, following
       http://mmacklin.com/smallsteps.pdf, we aren't solving the constraints with multiple iterations.
       Instead we divide the dt into small sub steps. This introduces us the following problems.

       Problem 1. Collision handling
       Dividing the time step can possibly mean the increased amount of collision detection per frame.
       We can negate this by doing the collision handling once at the start and hope for the best.

       Problem 2. Precision ('Solved' using the double precision)
       In XPBD, a lot of the equations include square(sub_step), which means that f32 might not be
       sufficient for calculation. For example, if the sub step gets higher than 100, the gravity will stop
       working due to lost precision. We can limit the number of sub steps(15~20), or use double, which 
       if fine for the CPU(not considering the cache), but bad for the GPU.

       All PBD starts with the linearization using the taylor expansion(gradient == partial differtiation).
       C(p + dp) (approx)= C(p) + gradient(C) * dp = 0 ...eq(1)
       Here, p includes the properties of all the particles that were involved in this constraint.
       So the eq(1) is equivalent to
       C(p0, p1 ...) + sum(gradient(C(pi))*dp(i)) = 0

       However, this equation is undetermined. For example, if the constraint was a distance constraint where
       the distance between the two particles should be maintained, there are endless possible positions.

       By limiting the direction of dp to the gradient(C(pi)), the equation can be solved.
       dp(i) = s*wi*gradient(C(pi)) ... eq(2), 
       where s is typically called a lagrange multiplier, and wi is a inverse mass of the particle(we'll get to this later).

       plugging eq(2) to eq(1), we get
       s = -C/sum(wi*square(gradient(C(pi)))). 

       Nice thing about having a inverse mass property in eq(2) is that the linear momentum is preserved.
       For the linear momentum to be preserved, the sum(mi*dp(i)) should be 0.
       Plugging eq(2) to it, we get s * sum(gradient(C(pi))), which is 0 due to translation invariance(Need to dig into this later).
       (What about the angular momentum?)
    */

    // TODO(gh) Need to think about how many sub step we need!
    f64 sub_dt = (f64)platform_input->dt_per_frame/(f64)tran_state->max_pbd_substep_count;
    f64 sub_dt_square = square(sub_dt);
    for(u32 i = 0;
        (i < tran_state->max_pbd_substep_count) && (tran_state->remaining_pbd_substep_count-- > 0);
        ++i)
    {
        u32 max_environment_constraint_count = 2048;
        TempMemory environment_constraint_memory = 
            start_temp_memory(&tran_state->transient_arena, 
                    sizeof(EnvironmentConstraint)*max_environment_constraint_count);
        EnvironmentConstraint *environment_constraints = 
            push_array(&environment_constraint_memory, EnvironmentConstraint, max_environment_constraint_count);
        u32 environment_constraint_count = 0;

        // NOTE(gh) Advance the positions of all the particles,
        // and generate environment constraint, since it is independent from other particles
        for(u32 entity_index = 0;
                entity_index < game_state->entity_count;
                ++entity_index)
        {
            Entity *entity = game_state->entities + entity_index;
            PBDParticleGroup *group = &entity->particle_group;
            if(group->count)
            {
                // Pre solve
                for(u32 particle_index = 0;
                        particle_index < group->count;
                        ++particle_index)
                {
                    PBDParticle *particle = group->particles + particle_index;
                    if(particle->inv_mass > 0.0f)
                    {
                        // TODO(gh) For damping, use the formula from XPBD which involves modified lagrange multiplier
                        particle->prev_p = particle->p;

                        // NOTE(gh) We no longer modify the velocity with external forces,
                        // but directly modify the position with second order
                        particle->p += sub_dt * particle->v + sub_dt_square*V3d(0, 0, -9.8);

                        if(particle->p.z < particle->r)
                        {
                            EnvironmentConstraint *c = environment_constraints + environment_constraint_count++;
                            c->particle = particle;
                            c->n = V3d(0, 0, 1);
                            c->d = 0;
                        }
                    }
                }
            }
        }

        u32 max_collision_constraint_count = 2048;
        TempMemory collision_constraint_memory = 
            start_temp_memory(&tran_state->transient_arena, 
                    sizeof(CollisionConstraint)*max_collision_constraint_count);
        CollisionConstraint *collision_constraints = 
            push_array(&collision_constraint_memory, CollisionConstraint, max_collision_constraint_count);
        u32 collision_constraint_count = 0;

        // NOTE(gh) Generate collision constraints
        for(u32 entity_index = 0;
                entity_index < game_state->entity_count;
                ++entity_index)
        {
            Entity *entity = game_state->entities + entity_index;
            PBDParticleGroup *group = &entity->particle_group;
            if(group->count)
            {
                for(u32 test_entity_index = 0;
                        test_entity_index < game_state->entity_count;
                        ++test_entity_index)
                {
                    Entity *test_entity = game_state->entities + test_entity_index;
                    if(is_entity_flag_set(test_entity, EntityFlag_Collides) &&
                       is_entity_flag_set(test_entity, EntityFlag_Movable))
                    {
                        PBDParticleGroup *test_group = &test_entity->particle_group;
                        if(test_entity > entity && test_group->count)
                        {
                            for(u32 particle_index = 0;
                                    particle_index < group->count;
                                    ++particle_index)
                            {
                                PBDParticle *particle = group->particles + particle_index;
                                for(u32 test_particle_index = 0;
                                        test_particle_index < test_group->count;
                                        ++test_particle_index)
                                {
                                    PBDParticle *test_particle = test_group->particles + test_particle_index;

                                    if(particle->inv_mass + test_particle->inv_mass != 0.0f)
                                    {
                                        f64 distance_between = length(particle->p - test_particle->p);
                                        if(distance_between < particle->r + test_particle->r)
                                        {
                                            CollisionConstraint *c = collision_constraints + collision_constraint_count++;
                                            c->particle0 = particle;
                                            c->particle1 = test_particle;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        u32 pre_stabilization_iter_count = 2;
        for(u32 iter = 0;
                iter < pre_stabilization_iter_count;
                ++iter)
        {
            // Pre-stabilize environment constraints
            for(u32 constraint_index = 0;
                    constraint_index < environment_constraint_count;
                    ++constraint_index)
            {
                EnvironmentConstraint *c = environment_constraints + constraint_index;
                EnvironmentSolution solution = {};
                solve_environment_constraint(&solution, c, &c->particle->prev_p, sub_dt);

                c->particle->p += solution.offset;
                c->particle->prev_p += solution.offset;
            }

            // Pre-stabilize collision constraints
            for(u32 constraint_index = 0;
                    constraint_index < collision_constraint_count;
                    ++constraint_index)
            {
                CollisionConstraint *c = collision_constraints + constraint_index;

                CollisionSolution solution = {};
                solve_collision_constraint(&solution, c,
                                           &c->particle0->prev_p, &c->particle1->prev_p, sub_dt);

                c->particle0->p += solution.offset0;
                c->particle0->prev_p += solution.offset0;

                c->particle1->p += solution.offset1;
                c->particle1->prev_p += solution.offset1;
            }
        }

        /*
           TODO(gh) Find out in what precise order we should solve the constraints,
           especially with the environment and collision
           Solve every constraints, in specific order.
           environment -> collision -> distance -> shape matching
           */

        // Solve environment constraints
        for(u32 constraint_index = 0;
                constraint_index < environment_constraint_count;
                ++constraint_index)
        {
            EnvironmentConstraint *c = environment_constraints + constraint_index;
            EnvironmentSolution solution = {};
            solve_environment_constraint(&solution, c, &c->particle->p, sub_dt);

           c->particle->p += solution.offset;
        }

        // Solve collision constraints & friction
        for(u32 constraint_index = 0;
                constraint_index < collision_constraint_count;
                ++constraint_index)
        {
            CollisionConstraint *c = collision_constraints + constraint_index;

            CollisionSolution solution = {};
            solve_collision_constraint(&solution, c,
                                       &c->particle0->p, &c->particle1->p, sub_dt);

            c->particle0->p += solution.offset0;
            c->particle1->p += solution.offset1;

            // TODO(gh) Friction seems busted...,
            // come back when we have SDF
#if 0
            if(solution.collided)
            {
                v3d d = (c->particle0->p - c->particle0->prev_p) - (c->particle1->p - c->particle1->prev_p);

                v3d tangential_displacement = d - dot(d, solution.contact_normal) * solution.contact_normal;
                f64 length_tangential_displacement = length(tangential_displacement);

                f64 static_coeff = 0.7;
                f64 kinetic_coeff = 0.4;
                v3d offset = V3d();
                if(length_tangential_displacement < static_coeff)
                {
                    offset =  tangential_displacement;
                }
                else
                {
                    offset = minimum(kinetic_coeff*solution.penetration_depth / length_tangential_displacement, 1.0) * 
                             tangential_displacement;
                }

                offset *= (c->particle0->inv_mass/(c->particle0->inv_mass + c->particle1->inv_mass));
                c->particle0->p += offset;
                c->particle1->p += (c->particle1->inv_mass/(c->particle0->inv_mass + c->particle1->inv_mass)) * (-offset);
            }
#endif

        }


#if 0
        // NOTE(gh) Solve distance constraint
        for(u32 entity_index = 0;
                entity_index < game_state->entity_countddddddddf;
                ++entity_index)
        {
            Entity *entity = game_state->entities + entity_index;
            PBDParticleGroup *group = &entity->particle_group;

            // Solve distance constraint, only used for cloth or chains
            for(u32 constraint_index = 0;
                    constraint_index < group->distance_constraint_count;
                    ++constraint_index)
            {
                DistanceConstraint *c = group->distance_constraints + constraint_index;
                PBDParticle *particle0 = group->particles + c->index0;
                PBDParticle *particle1 = group->particles + c->index1;

                if(particle0->inv_mass + particle1->inv_mass != 0.0f)
                {
                    v3d delta = particle0->p - particle1->p;
                    f64 delta_length = length(delta);

                    f64 C = delta_length - c->rest_length;

                    if(C < 0.0)
                    {
                        f64 stiffness_epsilon = (f64)group->inv_distance_stiffness/sub_dt_square;
                        solve_distance_constraint(particle0, particle1, delta, C, stiffness_epsilon);
                    }
                }
            }
        }
#endif
    
#if 1
        /*
           NOTE(gh) Solve shape matching constraints.
           http://www.beosil.com/download/MeshlessDeformations_SIG05.pdf

           The basic idea behind this is about finding a 'rotation matrix' that 
           when applied each of the initial offset from the COM, produces the 'goal position'
           that would match the shape.

           This _must_ be the last step, especially for the rigid bodies.

           1. Rigid bodies
           m3x3d A = sum((xi - com) * transpose(ri)),
           where xi is the position, and ri is the offset from the COM when resting.
           The rotational part, R, can be retrieved using the polar decomposition,
           which is explained in 
           https://matthias-research.github.io/pages/publications/stablePolarDecomp.pdf

           2. Linear deformation
           Similar to rigid bodies, but support stretching.

           3. Quadratic deformation

        */
        // TODO/IMPORTANT(gh) Make sure the polar decomposition math checks out!
        for(u32 entity_index = 0;
                entity_index < game_state->entity_count;
                ++entity_index)
        {
            Entity *entity = game_state->entities + entity_index;
            PBDParticleGroup *group = &entity->particle_group;

            if(is_entity_flag_set(entity, EntityFlag_RigidBody))
            {
                m3x3d A = get_shape_matching_rigid_body_deformation_matrix(group);

                group->shape_match_quat = 
                    extract_rotation_from_polar_decomposition(&A, &group->shape_match_quat, 32);
                m3x3d shape_matching_matrix = 
                    orientation_quatd_to_m3x3d(group->shape_match_quat);

                v3d com = get_com_of_particle_group(group);
                // Apply the shape matching rotation
                for(u32 particle_index = 0;
                        particle_index < group->count;
                        ++particle_index)
                {
                    PBDParticle *particle = group->particles + particle_index;
                    particle->p = shape_matching_matrix*particle->initial_offset_from_com + com;
                }
            }
            else if(is_entity_flag_set(entity, EntityFlag_Linear))
            {
                m3x3d shape_matching_matrix = 
                    get_shape_matching_linear_deformation_rotation_matrix(group);

                v3d com = get_com_of_particle_group(group);
                for(u32 particle_index = 0;
                        particle_index < group->count;
                        ++particle_index)
                {
                    PBDParticle *particle = group->particles + particle_index;

                    // NOTE(gh) This is also from the shape-matching paper
                    f64 alpha = sub_dt_square*group->linear_deformation_c * particle->inv_mass;
                    particle->p += alpha*(shape_matching_matrix*particle->initial_offset_from_com + com - particle->p);
                }
            }
            else if(is_entity_flag_set(entity, EntityFlag_Quadratic))
            {
                v3d com = get_com_of_particle_group(group);
                m3x9d quadratic_Apq = {};
                for(u32 particle_index = 0;
                        particle_index < group->count;
                        ++particle_index)
                {
                    PBDParticle *particle = group->particles + particle_index;

                    v3d offset = particle->p - com;
                    v9d q = get_quadratic_deformation_q(particle->initial_offset_from_com);

                    f64 particle_mass = 1.0/particle->inv_mass;

                    quadratic_Apq.rows[0] += particle_mass*offset.x * q;
                    quadratic_Apq.rows[1] += particle_mass*offset.y * q;
                    quadratic_Apq.rows[2] += particle_mass*offset.z * q;
                }

                m3x9d quadratic_A = quadratic_Apq*group->quadratic_inv_Aqq;

                // NOTE(gh) Quadratic deformation is done on top of 
                // linea deformation, so we need to caculate linear deformation first.
                // quadratric_R = [R 0 0], which results in 3x9 matrix
                m3x3d R = 
                    get_shape_matching_linear_deformation_rotation_matrix(group);
                m3x9d quadratric_R = {};
                quadratric_R.rows[0] = V9d(R.e[0][0], R.e[0][1], R.e[0][2],
                                            0, 0, 0, 0, 0, 0);
                quadratric_R.rows[1] = V9d(R.e[1][0], R.e[1][1], R.e[1][2],
                                            0, 0, 0, 0, 0, 0);
                quadratric_R.rows[2] = V9d(R.e[2][0], R.e[2][1], R.e[2][2],
                                            0, 0, 0, 0, 0, 0);

                f64 quadratic_coefficient = 0.5;
                m3x9d shape_match_rotation_matrix = quadratic_coefficient*quadratic_A + 
                                                    (1-quadratic_coefficient)*quadratric_R;

                for(u32 particle_index = 0;
                        particle_index < group->count;
                        ++particle_index)
                {
                    PBDParticle *particle = group->particles + particle_index;

                    v9d q = get_quadratic_deformation_q(particle->initial_offset_from_com);

                    particle->p = shape_match_rotation_matrix*q + com;
                }
            }
        }
#endif

        // Post solve
        for(u32 entity_index = 0;
                entity_index < game_state->entity_count;
                ++entity_index)
        {
            Entity *entity = game_state->entities + entity_index;
            PBDParticleGroup *group = &entity->particle_group;

            for(u32 particle_index = 0;
                    particle_index < group->count;
                    ++particle_index)
            {
                PBDParticle *particle = group->particles + particle_index;

                if(particle->inv_mass != 0.0f)
                {
                    v3d delta = (particle->p - particle->prev_p);
                    f64 sleep_epsilon = 0.00000001;
                    if(length(delta) > sleep_epsilon)
                    {
                        particle->v = delta / sub_dt;
                    }
                    else
                    {
                        // Particle sleeping
                        particle->p = particle->prev_p;
                    }
                }
            }
        }

        end_temp_memory(&collision_constraint_memory);
        end_temp_memory(&environment_constraint_memory);
    }

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
            grass_grid_index < tran_state->grass_grid_count_x*tran_state->grass_grid_count_y;
            ++grass_grid_index)
    {
        GrassGrid *grid = tran_state->grass_grids + grass_grid_index;

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
            tran_state->grass_grids, tran_state->grass_grid_count_x, tran_state->grass_grid_count_y, 
            V3(), true);
    platform_render_push_buffer->enable_shadow = true;
    platform_render_push_buffer->enable_grass_rendering = true;

    if(debug_platform_render_push_buffer)
    {
        init_render_push_buffer(debug_platform_render_push_buffer, render_camera, game_camera,
                    0, 0, 0, V3(), false);
    }

#if 0
    // NOTE(gh) Render all saved game states, especially the particles
    if(!tran_state->is_simulating_in_realtime)
    {
        u32 one_past_last_index = 0;
        if(tran_state->has_entire_buffer_filled_at_least_once)
        {
            one_past_last_index = tran_state->max_saved_game_state_count;
        }
        else
        {
            one_past_last_index = tran_state->saved_game_state_write_cursor;
        }

        // TODO(gh) This doesn't care about the 'order' now,
        // but we wanna differentiate the color based on the order later!!
        for(u32 game_state_index = 0;
                game_state_index < 5;
                ++game_state_index)
        {
            render_all_entities(platform_render_push_buffer, tran_state->saved_game_states + game_state_index, &tran_state->assets);
        }
    }
#endif
    render_all_entities(platform_render_push_buffer, game_state, &tran_state->assets, true, true);

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
        output_debug_records(debug_platform_render_push_buffer, &tran_state->assets, V2(0, 0));
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
#if 1
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
#endif
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
#if 1
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



