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

#define swap(ptr0, ptr1) {void *temp = (void*)(ptr0); ptr1 = ptr0; *(void **)(&ptr0) = temp;}

internal void
initialize_fluid_cube(FluidCube *fluid_cube, MemoryArena *arena, u32 cell_count_x, u32 cell_count_y, u32 cell_count_z, f32 cell_dim, f32 viscosity)
{
    fluid_cube->viscosity = viscosity;
    fluid_cube->cell_count = V3u(cell_count_x, cell_count_y, cell_count_z); 
    fluid_cube->cell_dim = cell_dim; 

    u32 total_cell_count = cell_count_x*cell_count_y*cell_count_z;

    fluid_cube->v_x = push_array(arena, f32, 2*total_cell_count); 
    fluid_cube->v_y = push_array(arena, f32, 2*total_cell_count); 
    fluid_cube->v_z = push_array(arena, f32, 2*total_cell_count); 
    fluid_cube->densities = push_array(arena, f32, 2*total_cell_count); 
    fluid_cube->pressures = push_array(arena, f32, total_cell_count); 
    zero_memory(fluid_cube->v_x, sizeof(f32)*2*total_cell_count);
    zero_memory(fluid_cube->v_y, sizeof(f32)*2*total_cell_count);
    zero_memory(fluid_cube->v_z, sizeof(f32)*2*total_cell_count);
    zero_memory(fluid_cube->densities, sizeof(f32)*2*total_cell_count);
    zero_memory(fluid_cube->pressures, sizeof(f32)*total_cell_count);

    fluid_cube->v_x_source = fluid_cube->v_x;
    fluid_cube->v_x_dest = fluid_cube->v_x_source + total_cell_count;
    fluid_cube->v_y_source = fluid_cube->v_y;
    fluid_cube->v_y_dest = fluid_cube->v_y_source + total_cell_count;
    fluid_cube->v_z_source = fluid_cube->v_z;
    fluid_cube->v_z_dest = fluid_cube->v_z_source + total_cell_count;

    fluid_cube->density_source = fluid_cube->densities;
    fluid_cube->density_dest = fluid_cube->density_source + total_cell_count;
}

// simply returns the index using x, y, z coordinate
internal u32
get_index(v3u cell_count, u32 cell_x, u32 cell_y, u32 cell_z)
{
    u32 result = cell_count.x*cell_count.y*cell_z + cell_count.x*cell_y + cell_x;
    assert(result < cell_count.x*cell_count.y*cell_count.z);
    return result;
}

internal void
add_force(f32 *dest, f32 *source, f32 *force, v3u cell_count, f32 dt)
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

                if(force[ID] != 0.0f)
                {
                    int a = 1;
                }

                dest[get_index(cell_count, cell_x, cell_y, cell_z)] = source[ID] + dt*force[ID];  
            }
        }
    }
}

// To use Hode decomposition, we should set the boundary
// This can be done in GPU by drawing a line
void set_boundary_values(f32 *dest, v3u cell_count, ElementTypeForBoundary element_type)
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
                switch(element_type)
                {
                    case ElementTypeForBoundary_xy:
                    {
                        dest[get_index(cell_count, cell_x, cell_y, 0)] = -dest[get_index(cell_count, cell_x, cell_y, 1)];
                        dest[get_index(cell_count, cell_x, cell_y, cell_count.z-1)] = -dest[get_index(cell_count, cell_x, cell_y, cell_count.z-2)];

                        if((cell_x == cell_count.x/2) && 
                            (cell_y == cell_count.y/2) &&
                            (cell_z == cell_count.z-2))
                        {
                            int a = 1;
                        }
                    }break;

                    case ElementTypeForBoundary_yz:
                    {
                        dest[get_index(cell_count, 0, cell_y, cell_z)] = -dest[get_index(cell_count, 1, cell_y, cell_z)];
                        dest[get_index(cell_count, cell_count.x-1, cell_y, cell_z)] = -dest[get_index(cell_count, cell_count.x-2, cell_y, cell_z)];

                    }break;

                    case ElementTypeForBoundary_zx:
                    {
                        dest[get_index(cell_count, cell_x, 0, cell_z)] = -dest[get_index(cell_count, cell_x, 1, cell_z)];
                        dest[get_index(cell_count, cell_x, cell_count.y-1, cell_z)] = -dest[get_index(cell_count, cell_x, cell_count.y-2, cell_z)];
                    }break;

                    case ElementTypeForBoundary_Continuous:
                    {
                        // TODO(gh) Original implementation doesn't seem to be doing this,
                        // do we really need to do this?
                        dest[get_index(cell_count, 0, cell_y, cell_z)] = dest[get_index(cell_count, 1, cell_y, cell_z)];
                        dest[get_index(cell_count, cell_count.x-1, cell_y, cell_z)] = dest[get_index(cell_count, cell_count.x-2, cell_y, cell_z)];
                        dest[get_index(cell_count, cell_x, 0, cell_z)] = dest[get_index(cell_count, cell_x, 1, cell_z)];
                        dest[get_index(cell_count, cell_x, cell_count.y-1, cell_z)] = dest[get_index(cell_count, cell_x, cell_count.y-2, cell_z)];
                        dest[get_index(cell_count, cell_x, cell_y, 0)] = dest[get_index(cell_count, cell_x, cell_y, 1)];
                        dest[get_index(cell_count, cell_x, cell_y, cell_count.z-1)] = dest[get_index(cell_count, cell_x, cell_y, cell_count.z-2)];
                    }break;
                }
            }
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

                v3 v = V3(v_x[ID], v_y[ID], v_z[ID]);

                v3 cell_based_u = v / cell_dim;

                // TODO(gh) Should we do 0.5f for xyz here?
                v3 p_offset = dt*cell_based_u;
                v3 p = V3(cell_x+0.5f, cell_y+0.5f, cell_z+0.5f) - p_offset;

                if(element_type == ElementTypeForBoundary_xy && v.z != 0.0f)
                {
                    int a = 1;
                }

                // clip p
                if(p.x < cell_dim + 0.5f)
                {
                    p.x = cell_dim + 0.5f;
                }
                else if(p.x > cell_count.x - 1.5f)
                {
                    p.x = cell_count.x - 1.5f;
                }

                if(p.y < cell_dim + 0.5f)
                {
                    p.y = cell_dim + 0.5f;
                }
                else if(p.y > cell_count.y - 1.5f)
                {
                    p.y = cell_count.y - 1.5f;
                }
                
                if(p.z < cell_dim + 0.5f)
                {
                    p.z = cell_dim + 0.5f;
                }
                else if(p.z > cell_count.z - 1.5f)
                {
                    p.z = cell_count.z - 1.5f;
                }

                u32 x0 = (u32)p.x;
                u32 x1 = x0+1;
                f32 xf = p.x - x0 - 0.5f;

                u32 y0 = (u32)p.y;
                u32 y1 = y0+1;
                f32 yf = p.y - y0 - 0.5f;

                u32 z0 = (u32)p.z;
                u32 z1 = z0+1;
                f32 zf = p.z - z0 - 0.5f;

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
    // dest = (a*source+dest[-1,0,0]+dest[1,0,0]+dest[0,-1,0]+dest[0,1,0],+dest[0,0,-1]+dest[0,0,1])/(6+a), 
    // where a = (dx^2)/(viscosity*dt).
    for(u32 iter = 0;
            iter < 40; // requires 40 to 80 iteration to make the error unnoticable
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

                    if((cell_x == cell_count.x/2) && 
                        (cell_y == cell_count.y/2) &&
                        (cell_z == cell_count.z-2) &&
                        element_type == ElementTypeForBoundary_xy)
                    {
                        int a = 1;
                    }

                    // NOTE(gh) To simplify the naming, we assume that 111 means the current cell.
                    f32 dest_011 = dest[get_index(cell_count, cell_x-1, cell_y, cell_z)];
                    f32 dest_211 = dest[get_index(cell_count, cell_x+1, cell_y, cell_z)];

                    f32 dest_101 = dest[get_index(cell_count, cell_x, cell_y-1, cell_z)];
                    f32 dest_121 = dest[get_index(cell_count, cell_x, cell_y+1, cell_z)];

                    f32 dest_110 = dest[get_index(cell_count, cell_x, cell_y, cell_z-1)];
                    f32 dest_112 = dest[get_index(cell_count, cell_x, cell_y, cell_z+1)];

                    // TODO(gh) Not sure if we should be using cell_dim or the whole cube dim here
                    f32 a = (cell_dim*cell_dim)/(viscosity*dt);
                    dest[ID] = (a*source[ID] + dest_011+dest_211+dest_101+dest_121+dest_110+dest_112)/(6+a);

                    if(dest[ID] != 0)
                    {
                        int a = 1;
                    }
                }
            }
        }

        set_boundary_values(dest, cell_count, element_type);
    }
}

// NOTE(gh) Project only works for vector components such as velocity,
// and components like density should not use this routine.
internal void
project(f32 *x, f32 *y, f32 *z, f32 *pressures, v3u cell_count, f32 cell_dim, f32 viscosity, f32 dt)
{
    TIMED_BLOCK();
    zero_memory(pressures, sizeof(f32)*cell_count.x*cell_count.y*cell_count.z);

    // First, get the divergence
    for(u32 iter = 0;
            iter < 40; // requires 40 to 80 iteration to make the error unnoticable
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
                    f32 p011 = pressures[get_index(cell_count, cell_x-1, cell_y, cell_z)];
                    f32 p211 = pressures[get_index(cell_count, cell_x+1, cell_y, cell_z)];

                    f32 p101 = pressures[get_index(cell_count, cell_x, cell_y-1, cell_z)];
                    f32 p121 = pressures[get_index(cell_count, cell_x, cell_y+1, cell_z)];

                    f32 p110 = pressures[get_index(cell_count, cell_x, cell_y, cell_z-1)];
                    f32 p112 = pressures[get_index(cell_count, cell_x, cell_y, cell_z+1)];
                    f32 divergence = (p211+p121+p112-p011-p101-p110)/(2*cell_dim);

                    // TODO(gh) Once this works, we can simplify this more
                    pressures[ID] = (p011+p211+p101+p121+p110+p112 - divergence*cell_dim*cell_dim)/6;
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
    set_boundary_values(x, cell_count, ElementTypeForBoundary_yz);
    set_boundary_values(y, cell_count, ElementTypeForBoundary_zx);
    set_boundary_values(z, cell_count, ElementTypeForBoundary_xy);
}

