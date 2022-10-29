/*
 * Written by Gyuhyun Lee
 */
// NOTE(gh) A lot the operations here can be benefited by the GPU and 
// might end up using it - which means some codes might get outdated.
// Please check the shaders alongside with this code to get the full picture!

#include "hb_fluid.h"
/*
   NOTE(gh) The fluid simulation takes these steps : 
   W0 -> W1 -> W2 -> W3 -> W4

   W0 -> W1 : add force 
   w1 = wo + dt*f

   W1 -> W2 : Advection
   w2 = w1(p(x, -dt))
   We go back in time to find what particle will end in the current cell.

   W2 -> W3 : Diffusion
   (I-v*dt*laplacian)w3 = w2;
   We need to solve the poisson equation for this.
   We try to find the diffusion value that when went back in time, becomes the diffusion value that we are holding.

   W3 -> W4 : Projection
   From the Hodge decomposition, we know that w = u + gradient, which means we can get u when we know w and gradient.
   If we take the divergence on both sides, we get div(w) = div(u) + laplacian(pressure)
   By definition, divergence of zero-divergence vector field is 0, so div(w) = laplacian(pressure), which is also a poisson equation.
*/

#define swap(ptr0, ptr1) {void *temp = (void*)(ptr0); ptr0 = ptr1; *(void **)(&ptr1) = temp;}

internal void
initialize_fluid_cube(FluidCube *fluid_cube, MemoryArena *arena, v3 left_bottom_p, u32 cell_count_x, u32 cell_count_y, u32 cell_count_z, f32 cell_dim, f32 viscosity)
{
    fluid_cube->left_bottom_p = left_bottom_p;
    fluid_cube->viscosity = viscosity;
    fluid_cube->cell_count = V3u(cell_count_x, cell_count_y, cell_count_z); 
    fluid_cube->cell_dim = cell_dim; 

    fluid_cube->total_cell_count = cell_count_x*cell_count_y*cell_count_z;
    fluid_cube->stride = sizeof(f32)*fluid_cube->total_cell_count;

    fluid_cube->v_x = push_array(arena, f32, 2*fluid_cube->total_cell_count); 
    fluid_cube->v_y = push_array(arena, f32, 2*fluid_cube->total_cell_count); 
    fluid_cube->v_z = push_array(arena, f32, 2*fluid_cube->total_cell_count); 
    fluid_cube->densities = push_array(arena, f32, 2*fluid_cube->total_cell_count); 
    fluid_cube->pressures = push_array(arena, f32, fluid_cube->total_cell_count); 

    zero_memory(fluid_cube->v_x, 2*fluid_cube->stride);
    zero_memory(fluid_cube->v_y, 2*fluid_cube->stride);
    zero_memory(fluid_cube->v_z, 2*fluid_cube->stride);
    zero_memory(fluid_cube->densities, 2*fluid_cube->stride);
    zero_memory(fluid_cube->pressures, fluid_cube->stride);

    fluid_cube->v_x_source = fluid_cube->v_x;
    fluid_cube->v_x_dest = fluid_cube->v_x_source + fluid_cube->total_cell_count;
    fluid_cube->v_y_source = fluid_cube->v_y;
    fluid_cube->v_y_dest = fluid_cube->v_y_source + fluid_cube->total_cell_count;
    fluid_cube->v_z_source = fluid_cube->v_z;
    fluid_cube->v_z_dest = fluid_cube->v_z_source + fluid_cube->total_cell_count;

    fluid_cube->density_source = fluid_cube->densities;
    fluid_cube->density_dest = fluid_cube->density_source + fluid_cube->total_cell_count;
}

// simply returns the index using x, y, z coordinate
internal u32
get_index(v3u cell_count, u32 cell_x, u32 cell_y, u32 cell_z)
{
    u32 result = cell_count.x*cell_count.y*cell_z + cell_count.x*cell_y + cell_x;
    assert(result < cell_count.x*cell_count.y*cell_count.z);
    return result;
}

// NOTE(gh) The source can be force, density, temperature... whatever you want the fluid to carry.
internal void
add_source(f32 *dest, f32 *source, f32 *force, v3u cell_count, f32 dt)
{
    // NOTE(gh) Original implementation allows force to be applied to the boundary?
    for(u32 cell_z = 0;
            cell_z < cell_count.z;
            ++cell_z)
    {
        for(u32 cell_y = 0;
                cell_y < cell_count.y;
                ++cell_y)
        {
            for(u32 cell_x = 0;
                    cell_x < cell_count.x;
                    ++cell_x)
            {
                u32 ID = get_index(cell_count, cell_x, cell_y, cell_z);

                dest[ID] = source[ID] + dt*force[ID];  
            }
        }
    }
}

// To use Hode decomposition, we should set the boundary
// This can be done in GPU by drawing a line
void set_boundary_values(f32 *dest, v3u cell_count, ElementTypeForBoundary element_type)
{
    TIMED_BLOCK();
    
    // NOTE(gh) We should flip X in two YZ planes where x == 0 || x == cell_count.x-1,
    // but keep the other values same
    for(u32 cell_z = 0;
            cell_z < cell_count.z;
            ++cell_z)
    {
        for(u32 cell_y = 0;
                cell_y < cell_count.y;
                ++cell_y)
        {
            dest[get_index(cell_count, 0, cell_y, cell_z)] = (element_type == ElementTypeForBoundary_NegateX) ?
                -dest[get_index(cell_count, 1, cell_y, cell_z)] : dest[get_index(cell_count, 1, cell_y, cell_z)];
            dest[get_index(cell_count, cell_count.x-1, cell_y, cell_z)] = (element_type == ElementTypeForBoundary_NegateX) ?
                -dest[get_index(cell_count, cell_count.x-2, cell_y, cell_z)] : dest[get_index(cell_count, cell_count.x-2, cell_y, cell_z)];
        }
    }

    // NOTE(gh) We should flip Y in two XZ planes where y == 0 || y == cell_count.y-1
    // but keep the other values same
    for(u32 cell_z = 0;
            cell_z < cell_count.z;
            ++cell_z)
    {
        for(u32 cell_x = 0;
                cell_x < cell_count.x;
                ++cell_x)
        {
            dest[get_index(cell_count, cell_x, 0, cell_z)] = (element_type == ElementTypeForBoundary_NegateY) ?
                -dest[get_index(cell_count, cell_x, 1, cell_z)] : dest[get_index(cell_count, cell_x, 1, cell_z)];
            dest[get_index(cell_count, cell_x, cell_count.y-1, cell_z)] = (element_type == ElementTypeForBoundary_NegateY) ?
                -dest[get_index(cell_count, cell_x, cell_count.y-2, cell_z)] : dest[get_index(cell_count, cell_x, cell_count.y-2, cell_z)];
        }
    }

    // NOTE(gh) We should flip Z in two XY planes where z == 0 || z == cell_count.z-1
    // but keep the other values same
    for(u32 cell_y = 0;
            cell_y < cell_count.y;
            ++cell_y)
    {
        for(u32 cell_x = 0;
                cell_x < cell_count.x;
                ++cell_x)
        {
            dest[get_index(cell_count, cell_x, cell_y, 0)] = (element_type == ElementTypeForBoundary_NegateZ) ?
                -dest[get_index(cell_count, cell_x, cell_y, 1)] : dest[get_index(cell_count, cell_x, cell_y, 1)];
            dest[get_index(cell_count, cell_x, cell_y, cell_count.z-1)] = (element_type == ElementTypeForBoundary_NegateZ) ?
                -dest[get_index(cell_count, cell_x, cell_y, cell_count.z-2)] : dest[get_index(cell_count, cell_x, cell_y, cell_count.z-2)];
        }
    }

    // Set the corner values, this implementation is from Fluid for Games by Jos Stam
    // bottom z 4 corners 
    dest[get_index(cell_count, 0, 0, 0)] = (dest[get_index(cell_count, 1, 0, 0)]+
                                            dest[get_index(cell_count, 0, 1, 0)]+
                                            dest[get_index(cell_count, 0, 0, 1)])/3;
    dest[get_index(cell_count, cell_count.x-1, 0, 0)] = (dest[get_index(cell_count, cell_count.x-2, 0, 0)]+
                                                        dest[get_index(cell_count, cell_count.x-1, 1, 0)]+
                                                        dest[get_index(cell_count, cell_count.x-1, 0, 1)])/3;

    dest[get_index(cell_count, 0, cell_count.y-1, 0)] = (dest[get_index(cell_count, 1, cell_count.y-1, 0)]+
                                                        dest[get_index(cell_count, 0, cell_count.y-2, 0)]+
                                                        dest[get_index(cell_count, 0, cell_count.y-1, 1)])/3;
    dest[get_index(cell_count, cell_count.x-1, cell_count.y-1, 0)] = (dest[get_index(cell_count, cell_count.x-2, cell_count.y-1, 0)]+
                                                        dest[get_index(cell_count, cell_count.x-1, cell_count.y-2, 0)]+
                                                        dest[get_index(cell_count, cell_count.x-1, cell_count.y-1, 1)])/3;


    // top z 4 corners
    dest[get_index(cell_count, 0, 0, cell_count.z-1)] = (dest[get_index(cell_count, 1, 0, cell_count.z-1)]+
                                                        dest[get_index(cell_count, 0, 1, cell_count.z-1)]+
                                                        dest[get_index(cell_count, 0, 0, cell_count.z-2)])/3;
    dest[get_index(cell_count, cell_count.x-1, 0, cell_count.z-1)] = (dest[get_index(cell_count, cell_count.x-2, 0, cell_count.z-1)]+
                                                        dest[get_index(cell_count, cell_count.x-1, 1, cell_count.z-1)]+
                                                        dest[get_index(cell_count, cell_count.x-1, 0, cell_count.z-2)])/3;

    dest[get_index(cell_count, 0, cell_count.y-1, cell_count.z-1)] = (dest[get_index(cell_count, 1, cell_count.y-1, cell_count.z-1)]+
                                                                    dest[get_index(cell_count, 0, cell_count.y-2, cell_count.z-1)]+
                                                                    dest[get_index(cell_count, 0, cell_count.y-1, cell_count.z-2)])/3;
    dest[get_index(cell_count, cell_count.x-1, cell_count.y-1, cell_count.z-1)] = (dest[get_index(cell_count, cell_count.x-2, cell_count.y-1, cell_count.z-1)]+
                                                                                    dest[get_index(cell_count, cell_count.x-1, cell_count.y-2, cell_count.z-1)]+
                                                                                    dest[get_index(cell_count, cell_count.x-1, cell_count.y-1, cell_count.z-2)])/3;
}

internal void
advect(f32 *dest, f32 *source, f32 *v_x, f32 *v_y, f32 *v_z, v3u cell_count, f32 cell_dim, f32 dt, ElementTypeForBoundary element_type)
{
    TIMED_BLOCK();

    for(u32 cell_z = 1;
            cell_z < cell_count.z-1;
            ++cell_z)
    {
        for(u32 cell_y = 1;
                cell_y < cell_count.y-1;
                ++cell_y)
        {
            for(u32 cell_x = 1;
                    cell_x < cell_count.x-1;
                    ++cell_x)
            {
                u32 ID = get_index(cell_count, cell_x, cell_y, cell_z);
                if(ID == get_index(cell_count, cell_count.x/2, 1, cell_count.z/2) &&
                        element_type == ElementTypeForBoundary_Continuous)
                {
                    if(source[ID] != 0)
                    {
                        int a =1;
                    }
                }

                v3 v = V3(v_x[ID], v_y[ID], v_z[ID]);

                v3 cell_based_u = v/cell_dim;

                v3 p_offset = dt*cell_based_u;

                // NOTE(gh) No need to add 0.5f here, we will gonna starting lerp from the bottom left corner anyway
                // by casting (u32) to the position.
                v3 p = V3(cell_x, cell_y, cell_z) - p_offset;

                // TODO(gh) The range of clamping is different with the original implementation
                // clip p
                p.x = clamp(0.0f, p.x, cell_count.x-2.f);
                p.y = clamp(0.0f, p.y, cell_count.y-2.f);
                p.z = clamp(0.0f, p.z, cell_count.z-2.f);

                u32 x0 = (u32)p.x;
                u32 x1 = x0+1;
                f32 xf = p.x - x0;

                u32 y0 = (u32)p.y;
                u32 y1 = y0+1;
                f32 yf = p.y - y0;

                u32 z0 = (u32)p.z;
                u32 z1 = z0+1;
                f32 zf = p.z - z0;

                f32 lerp_x0 = lerp(source[get_index(cell_count, x0, y0, z0)], xf, source[get_index(cell_count, x1, y0, z0)]);
                f32 lerp_x1 = lerp(source[get_index(cell_count, x0, y1, z0)], xf, source[get_index(cell_count, x1, y1, z0)]);
                f32 lerp_x2 = lerp(source[get_index(cell_count, x0, y0, z1)], xf, source[get_index(cell_count, x1, y0, z1)]);
                f32 lerp_x3 = lerp(source[get_index(cell_count, x0, y1, z1)], xf, source[get_index(cell_count, x1, y1, z1)]);

                f32 lerp_y0 = lerp(lerp_x0, yf, lerp_x1);
                f32 lerp_y1 = lerp(lerp_x2, yf, lerp_x3);

                f32 lerp_xyz = lerp(lerp_y0, zf, lerp_y1);

                dest[ID] = lerp_xyz; 
            }
        }
    }

    set_boundary_values(dest, cell_count, element_type);
}

internal void
diffuse(f32 *dest, f32 *source, v3u cell_count, f32 cell_dim, f32 viscosity, f32 dt, ElementTypeForBoundary element_type)
{
    TIMED_BLOCK();
    // NOTE(gh) This is a solution for a poisson equation (I - v*dt*laplacian)dest = source 
    // using Jacobi iteration.
    // Jacobi iteration is in a form of 
    // dest = (a*source+dest[-1,0,0]+dest[1,0,0]+dest[0,-1,0]+dest[0,1,0]+dest[0,0,-1]+dest[0,0,1])/(6+a), 
    // where a = (dx^2)/(viscosity*dt).
    for(u32 iter = 0;
            iter < 128; // requires 40 to 80 iteration to make the error unnoticable
            ++iter)
    {
        for(u32 cell_z = 1;
                cell_z < cell_count.z-1;
                ++cell_z)
        {
            for(u32 cell_y = 1;
                    cell_y < cell_count.y-1;
                    ++cell_y)
            {
                for(u32 cell_x = 1;
                        cell_x < cell_count.x-1;
                        ++cell_x)
                {
                    u32 ID = get_index(cell_count, cell_x, cell_y, cell_z);

                    // NOTE(gh) using dest instead of source!
                    f32 dest_pos_x = dest[get_index(cell_count, cell_x+1, cell_y, cell_z)];
                    f32 dest_neg_x = dest[get_index(cell_count, cell_x-1, cell_y, cell_z)];

                    f32 dest_pos_y = dest[get_index(cell_count, cell_x, cell_y+1, cell_z)];
                    f32 dest_neg_y = dest[get_index(cell_count, cell_x, cell_y-1, cell_z)];

                    f32 dest_pos_z = dest[get_index(cell_count, cell_x, cell_y, cell_z+1)];
                    f32 dest_neg_z = dest[get_index(cell_count, cell_x, cell_y, cell_z-1)];

                    // TODO(gh) Not sure if we should be using cell_dim or the whole cube dim here
                    f32 a = (cell_dim*cell_dim)/(viscosity*dt);

                    f32 diffuse = (a*source[ID] + dest_pos_x+dest_neg_x+dest_pos_y+dest_neg_y+dest_pos_z+dest_neg_z)/(6+a);
                    dest[ID] = diffuse;
                }
            }
        }

        set_boundary_values(dest, cell_count, element_type);
    }
}

internal f32 
get_divergence(f32 *x, f32 *y, f32 *z, v3u cell_count, f32 cell_dim, u32 cell_x, u32 cell_y, u32 cell_z)
{
    u32 pos_xID = get_index(cell_count, cell_x+1, cell_y, cell_z);
    u32 neg_xID = get_index(cell_count, cell_x-1, cell_y, cell_z);

    u32 pos_yID = get_index(cell_count, cell_x, cell_y+1, cell_z);
    u32 neg_yID = get_index(cell_count, cell_x, cell_y-1, cell_z);

    u32 pos_zID = get_index(cell_count, cell_x, cell_y, cell_z+1);
    u32 neg_zID = get_index(cell_count, cell_x, cell_y, cell_z-1);

    f32 pos_x = x[pos_xID];
    f32 neg_x = x[neg_xID];

    f32 pos_y = y[pos_yID];
    f32 neg_y = y[neg_yID];

    f32 pos_z = z[pos_zID];
    f32 neg_z = z[neg_zID];

    f32 result = (pos_x+pos_y+pos_z-neg_x-neg_y-neg_z)/(2*cell_dim);

    return result;
}

// NOTE(gh) Project only works for vector components such as velocity,
// and components like density should not use this routine.
internal void
project(f32 *x, f32 *y, f32 *z, f32 *pressures, f32 *divergences, v3u cell_count, f32 cell_dim, f32 dt)
{
    TIMED_BLOCK();
    zero_memory(pressures, sizeof(f32)*cell_count.x*cell_count.y*cell_count.z);

    // First, get the divergence
    for(u32 cell_z = 1;
            cell_z < cell_count.z-1;
            ++cell_z)
    {
        for(u32 cell_y = 1;
                cell_y < cell_count.y-1;
                ++cell_y)
        {
            for(u32 cell_x = 1;
                    cell_x < cell_count.x-1;
                    ++cell_x)
            {
                u32 ID = get_index(cell_count, cell_x, cell_y, cell_z);
                
                f32 divergence = get_divergence(x, y, z, cell_count, cell_dim, cell_x, cell_y, cell_z);
                divergences[ID] = divergence;
            }
        }
    }
    set_boundary_values(divergences, cell_count, ElementTypeForBoundary_Continuous);

    for(u32 iter = 0;
            iter < 128; // requires 40 to 80 iteration to make the error unnoticable
            ++iter)
    {
        for(u32 cell_z = 1;
                cell_z < cell_count.z-1;
                ++cell_z)
        {
            for(u32 cell_y = 1;
                    cell_y < cell_count.y-1;
                    ++cell_y)
            {
                for(u32 cell_x = 1;
                        cell_x < cell_count.x-1;
                        ++cell_x)
                {
                    u32 ID = get_index(cell_count, cell_x, cell_y, cell_z);
                    
                    f32 p_pos_x = pressures[get_index(cell_count, cell_x+1, cell_y, cell_z)];
                    f32 p_neg_x = pressures[get_index(cell_count, cell_x-1, cell_y, cell_z)];

                    f32 p_pos_y = pressures[get_index(cell_count, cell_x, cell_y+1, cell_z)];
                    f32 p_neg_y = pressures[get_index(cell_count, cell_x, cell_y-1, cell_z)];

                    f32 p_pos_z = pressures[get_index(cell_count, cell_x, cell_y, cell_z+1)];
                    f32 p_neg_z = pressures[get_index(cell_count, cell_x, cell_y, cell_z-1)];

                    f32 divergence = divergences[ID];
                    // TODO(gh) Once this works, we can simplify this more
                    pressures[ID] = (p_pos_x+p_neg_x+p_pos_y+p_neg_y+p_pos_z+p_neg_z - divergence*cell_dim*cell_dim)/6;
                }
            }
        }
         
        set_boundary_values(pressures, cell_count, ElementTypeForBoundary_Continuous);
    }

    // Using the Hodge decomposition within the Neumann boundary, 
    // we know that every vector field can be represented with w = u + gradient(q),
    // where w is a vector field with divergence and u is a vector field without divergence.
    // To keep the simulation stable, we need to use u instead of w, but the Navier-Stokes equation that we used produce w
    // which means we need to subtract the gradient from it.
    for(u32 cell_z = 1;
            cell_z < cell_count.z-1;
            ++cell_z)
    {
        for(u32 cell_y = 1;
                cell_y < cell_count.y-1;
                ++cell_y)
        {
            for(u32 cell_x = 1;
                    cell_x < cell_count.x-1;
                    ++cell_x)
            {
                u32 ID = get_index(cell_count, cell_x, cell_y, cell_z);

                // TODO(gh) Not sure if we should be using cell_dim or the whole cube dim here
                f32 gradient_x = (pressures[get_index(cell_count, cell_x+1, cell_y, cell_z)] - pressures[get_index(cell_count, cell_x-1, cell_y, cell_z)]) /
                                (2*cell_dim);
                f32 gradient_y = (pressures[get_index(cell_count, cell_x, cell_y+1, cell_z)] - pressures[get_index(cell_count, cell_x, cell_y-1, cell_z)]) /
                                (2*cell_dim);
                f32 gradient_z = (pressures[get_index(cell_count, cell_x, cell_y, cell_z+1)] - pressures[get_index(cell_count, cell_x, cell_y, cell_z-1)]) /
                                (2*cell_dim);
                x[ID] -= gradient_x;
                y[ID] -= gradient_y;
                z[ID] -= gradient_z;
            }
        }
    }
    set_boundary_values(x, cell_count, ElementTypeForBoundary_NegateX);
    set_boundary_values(y, cell_count, ElementTypeForBoundary_NegateY);
    set_boundary_values(z, cell_count, ElementTypeForBoundary_NegateZ);
}

internal void
validate_divergence(f32 *x, f32 *y, f32 *z, v3u cell_count, f32 cell_dim)
{
    for(u32 cell_z = 1;
            cell_z < cell_count.z-1;
            ++cell_z)
    {
        for(u32 cell_y = 1;
                cell_y < cell_count.y-1;
                ++cell_y)
        {
            for(u32 cell_x = 1;
                    cell_x < cell_count.x-1;
                    ++cell_x)
            {
                f32 divergence = get_divergence(x, y, z, cell_count, cell_dim, cell_x, cell_y, cell_z);
                assert(divergence < 0.5);
            }
        }
    }
}

