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
   Because we are advancing not entirely smoothly like in nature but in a fixed time stamp(dt), the result at this point
   has divergence in it, which eventually makes the simulation unstable.
   From the Hodge decomposition, we know that w = u + gradient, which means we can get u when we know w and gradient.
   If we take the divergence on both sides, we get div(w) = div(u) + laplacian(pressure)
   By definition, divergence of zero-divergence vector field is 0, so div(w) = laplacian(pressure), which is also a poisson equation.
*/

/*
   NOTE(gh)Terms used in fluid simulation
   Gradient - vector that points at the higher value in scalar field.
   For example, if point A has higer pressure than point B, the gradient vector will point from B to A(vector BA)
   This is why the advection step in navier-stokes equation looks like : -(u*del)u, which means if the gradient vector is
   in opposite direction, the point that we are calculating this equation will end up getting higher value(velocity, density..)
   because the point with higer value will end replacing the current point in micro scale.
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
    fluid_cube->temp_buffer = push_array(arena, f32, fluid_cube->total_cell_count); 

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
// NOTE(gh) For velocity, remember that du/dt = ddp, because the change of velocity is acceleration.
// This is why we need to multiply the ddp by dt and then add it to the resulting velocity.
internal void
add_source(f32 *dest, f32 *source, f32 *ddp, v3u cell_count, f32 dt)
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

                dest[ID] = source[ID] + dt*ddp[ID];  
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
reverse_advect(f32 *dest, f32 *source, f32 *v_x, f32 *v_y, f32 *v_z, v3u cell_count, f32 cell_dim, f32 dt, ElementTypeForBoundary element_type)
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
forward_advect(f32 *dest, f32 *source, f32 *temp, f32 *v_x, f32 *v_y, f32 *v_z, v3u cell_count, f32 cell_dim, f32 dt, ElementTypeForBoundary element_type)
{
    TIMED_BLOCK();
    u32 stride =  sizeof(f32)*cell_count.x*cell_count.y*cell_count.z;
    zero_memory(dest, stride);
    zero_memory(temp, stride);
    memcpy(temp, source, stride);

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
                if(source[ID] != 0)
                {
                    int a= 1;
                }
                if(ID == get_index(cell_count, cell_count.x/2, 1, cell_count.z/2) &&
                        element_type == ElementTypeForBoundary_Continuous)
                {
                    if(temp[ID] != 0)
                    {
                        int a =1;
                    }
                }

                v3 v = V3(v_x[ID], v_y[ID], v_z[ID]);

                v3 cell_based_u = v/cell_dim;

                v3 p_offset = dt*cell_based_u;

                // NOTE(gh) No need to add 0.5f here, we will gonna starting lerp from the bottom left corner anyway
                // by casting (u32) to the position.
                v3 p = V3(cell_x, cell_y, cell_z) + p_offset;

                // TODO(gh) The range of clamping is different with the original implementation
                // TODO(gh) Is the range still valid in forward advection?
                p.x = clamp(0.0f, p.x, cell_count.x-2.f);
                p.y = clamp(0.0f, p.y, cell_count.y-2.f);
                p.z = clamp(0.0f, p.z, cell_count.z-2.f);

                u32 x0 = (u32)p.x;
                u32 x1 = x0+1;
                f32 xf = p.x - x0;
                f32 one_minus_xf = 1-xf;

                u32 y0 = (u32)p.y;
                u32 y1 = y0+1;
                f32 yf = p.y - y0;
                f32 one_minus_yf = 1-yf;

                u32 z0 = (u32)p.z;
                u32 z1 = z0+1;
                f32 zf = p.z - z0;
                f32 one_minus_zf = 1-zf;

                // TODO(gh) once this works, we can simplify the math here
                f32 value000 = xf*yf*zf*temp[ID];
                f32 value100 = one_minus_xf*yf*zf*temp[ID];
                f32 value010 = xf*one_minus_yf*zf*temp[ID];
                f32 value110 = one_minus_xf*one_minus_yf*zf*temp[ID];
                f32 value001 = xf*yf*one_minus_zf*temp[ID];
                f32 value101 = one_minus_xf*yf*one_minus_zf*temp[ID];
                f32 value011 = xf*one_minus_yf*one_minus_zf*temp[ID];
                f32 value111 = one_minus_xf*one_minus_yf*one_minus_zf*temp[ID];
#if 1
                dest[get_index(cell_count, x0, y0, z0)] += value000;
                dest[get_index(cell_count, x1, y0, z0)] += value100;
                dest[get_index(cell_count, x0, y1, z0)] += value010;
                dest[get_index(cell_count, x1, y1, z0)] += value110;
                dest[get_index(cell_count, x0, y0, z1)] += value001;
                dest[get_index(cell_count, x1, y0, z1)] += value101;
                dest[get_index(cell_count, x0, y1, z1)] += value011;
                dest[get_index(cell_count, x1, y1, z1)] += value111;
#endif

                temp[ID] = 0;
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






























































internal void
initialize_fluid_cube_mac(FluidCubeMAC *cube, MemoryArena *arena, v3 left_bottom_p, v3i cell_count, f32 cell_dim)
{
    cube->left_bottom_p = left_bottom_p;
    cube->cell_count = cell_count;
    cube->cell_dim = cell_dim;

    cube->v_x = push_array(arena, f32, 2*(cell_count.x+1)*cell_count.y*cell_count.z);
    cube->v_y = push_array(arena, f32, 2*cell_count.x*(cell_count.y+1)*cell_count.z);
    cube->v_z = push_array(arena, f32, 2*cell_count.x*cell_count.y*(cell_count.z+1));
    
    cube->pressures = push_array(arena, f32, cell_count.x*cell_count.y*cell_count.z);
    cube->densities = push_array(arena, f32, cell_count.x*cell_count.y*cell_count.z);
}

struct MACID
{
    // ID0 is smaller than ID1, no matter what axis that it's opertating on
    i32 ID0;
    i32 ID1;
};

// TODO(gh) Should double-check these functions
internal MACID
get_index_mac_x(i32 x, i32 y, i32 z, v3i cell_count)
{
    MACID result = {};
    result.ID0 = cell_count.y*(cell_count.x+1)*z + (cell_count.x+1)*y + x;
    result.ID1 = result.ID0+1;
    return result;
}

internal MACID
get_index_mac_y(i32 x, i32 y, i32 z, v3i cell_count)
{
    MACID result = {};
    result.ID0 = cell_count.x*(cell_count.y+1)*z + (cell_count.y+1)*x + y;
    result.ID1 = result.ID0+1;
    return result;
}

internal MACID
get_index_mac_z(i32 x, i32 y, i32 z, v3i cell_count)
{
    MACID result = {};
    result.ID0 = cell_count.x*(cell_count.z+1)*y + (cell_count.z+1)*x + z;
    result.ID1 = result.ID0+1;
    return result;
}

internal i32
get_index_mac_center(i32 x, i32 y, i32 z, v3i cell_count)
{
    i32 result = cell_count.x*cell_count.y*z + cell_count.x*y + x;
    return result;
}

// NOTE(gh) 'input' depends on what quantity we are dealing with at the momemnt.
// For velocity, it should be the acceleration(ddp), which will be multiplied by dt and added to the destination.
// Recalling the momentum equation, the 'mass' of the fluid doesn't matter, because it gets cancelled out 
// during getting du/dt
internal void
add_ddp(f32 *dest, f32 *source, f32 *input, v3i cell_count, v3i mac_offset, v3 cell_dim, u32 indicator)
{
    // Depending on x, y, and z, mac_offset should be (1,0,0),(0,1,0),(0,0,1) respectively
    for(u32 z = 0;
            z < cell_count.z;
            ++z)
    {
        for(u32 y = 0;
                y < cell_count.y;
                ++y)
        {
            for(u32 x = 0;
                    x < cell_count.x;
                    ++x)
            {
                MACID macID = {};
                switch(indicator)
                {
                    case 0:
                    {
                        macID = get_index_mac_x(x, y, z, cell_count);
                    }break;

                    case 1:
                    {
                        macID = get_index_mac_y(x, y, z, cell_count);
                    }break;

                    case 2:
                    {
                        macID = get_index_mac_z(x, y, z, cell_count);
                    }break;

                    default:
                    {
                        invalid_code_path;
                    };
                }

                dest[macID.ID0] += 0.5f*input[macID.ID0];
                dest[macID.ID1] += 0.5f*input[macID.ID1];
            }
        }
    }
}

internal f32
get_neighboring_pressure_with_boundary_condition(f32 *pressures, i32 center_x, i32 center_y, i32 center_z, v3i offset, v3i cell_count)
{
    f32 result = 0;

    i32 x = center_x+offset.x;
    i32 y = center_y+offset.y;
    i32 z = center_z+offset.z;

    b32 is_solid_wall = false;
    if(x < 0 || x >= cell_count.x ||
        y < 0 || y >= cell_count.y ||
        z < 0 || z >= cell_count.z)
    {
        // TODO(gh) For now, we will assume that the fluid cube is 
        // covered with solid walls for now(which sets the pressure to the center cell)
        // but we should also handle free-space case, which will just set the pressure to 0.
        is_solid_wall = true;
    }

    if(is_solid_wall)
    {
        // Set the pressure to the center P, which effectively negates 1 from the coefficient of center P in the equation
        // (4 in 2D, 6 in 3D)
        result = pressures[get_index_mac_center(x, y, z, cell_count)];
    }
    else
    {
        result = pressures[get_index_mac_center(x+offset.x, y+offset.y, z+offset.z, cell_count)];
    }

    return result;
}

enum BoundaryDirection
{
    BoundaryDirection_x, // u
    BoundaryDirection_y, // v
    BoundaryDirection_z, // w
};

// NOTE(gh) N-S mementum equation does have a pressure term (-delP/density),
// but we don't know the pressure that will satisfy the continuity equation (divergence(u) = 0)
// So we need to get the pressure, and subtract it from the result of the N-S equation
internal void
project_and_enforce_boundary_condition(f32 *dest, f32 *source, f32 *v_x, f32 *v_y, f32 *v_z, 
                                        f32 *pressures, f32 *temp_buffer,
                                        v3i cell_count, f32 cell_dim, f32 dt, BoundaryDirection boundary_direction)
{
    // TODO(gh) For now, we will assume that the density is uniform across the board.
    // Later, we would want to do something else(AKA smoke)
    f32 density = 1;
    zero_memory(pressures, sizeof(f32)*cell_count.x*cell_count.y*cell_count.z);
    zero_memory(temp_buffer, sizeof(f32)*cell_count.x*cell_count.y*cell_count.z);

    // As we know that divergence should be 0, we can express it in mac grid.
    // Then, we can express each u or v with u(zero-div)= u(yes-div) - (density/dt)*gradient(P)
    // This works even if the cell was occupied by a solid wall, because we can think
    // of a solid wall having a 'ghost' pressure.
    // The result looks like : P = ((-cell_dim*density/dt)*Divergence(u(yes-div)) + all of neighboring P)/6
    for(i32 z = 0;
            z < cell_count.z;
            ++z)
    {
        for(i32 y = 0;
                y < cell_count.y;
                ++y)
        {
            for(i32 x = 0;
                    x < cell_count.x;
                    ++x)
            {
                MACID macID_x = get_index_mac_x(x, y, z, cell_count);
                MACID macID_y = get_index_mac_y(x, y, z, cell_count);
                MACID macID_z = get_index_mac_z(x, y, z, cell_count);

                // TODO(gh) Replace all 0 with the solid wall velocity
                // NOTE(gh) This is (-cell_dim*cell_dim*density/dt)*Divergence(u(yes-div)) - (cell_dim*density/dt)*(u - u)(soild-fluid or fluid-solid, depending on where the solid wall was located) 
                // for each solid wall boundary)
                f32 a = -(cell_dim*density/dt);
                f32 rhs = a*(v_x[macID_x.ID1]-v_x[macID_x.ID0]+v_y[macID_y.ID1]-v_y[macID_y.ID0]+v_z[macID_z.ID1]-v_z[macID_z.ID0]); 
                 
                if(x == 0)
                {
                    rhs -= a*(v_x[macID_x.ID0] - 0);
                }
                else if(x == cell_count.x-1)
                {
                    rhs -= a*(0 - v_x[macID_y.ID1]);
                }

                if(y == 0)
                {
                    rhs -= a*(v_y[macID_y.ID0] - 0);
                }
                else if(y == cell_count.y-1)
                {
                    rhs -= a*(0 - v_y[macID_y.ID1]);
                }

                if(z == 0)
                {
                    rhs -= a * (v_z[macID_z.ID0] - 0);
                }
                else if(z == cell_count.z-1)
                {
                    rhs -= a*(0 - v_z[macID_z.ID1]);
                }

                temp_buffer[get_index_mac_center(x, y, z, cell_count)] = rhs;
            }
        }
    }

    // TODO(gh) We will stick with the basic Jacobi iteration for now, but we might wanna
    // switch to a better and faster converging algorithm.
    for(u32 iter = 0;
            iter < 40;
            ++iter)
    {
        for(i32 z = 0;
                z < cell_count.z;
                ++z)
        {
            for(i32 y = 0;
                    y < cell_count.y;
                    ++y)
            {
                for(i32 x = 0;
                        x < cell_count.x;
                        ++x)
                {
                    u32 centerID = get_index_mac_center(x, y, z, cell_count);
                    pressures[centerID] = 
                        (1.0f/6.0f)*(temp_buffer[centerID]+
                                    get_neighboring_pressure_with_boundary_condition(pressures, x, y, z, V3i(1, 0, 0), cell_count) +
                                    get_neighboring_pressure_with_boundary_condition(pressures, x, y, z, V3i(-1, 0, 0), cell_count) +
                                    get_neighboring_pressure_with_boundary_condition(pressures, x, y, z, V3i(0, 1, 0), cell_count) +
                                    get_neighboring_pressure_with_boundary_condition(pressures, x, y, z, V3i(0, -1, 0), cell_count) +
                                    get_neighboring_pressure_with_boundary_condition(pressures, x, y, z, V3i(0, 0, 1), cell_count) +
                                    get_neighboring_pressure_with_boundary_condition(pressures, x, y, z, V3i(0, 0, -1), cell_count));
                }
            }
        }
    }

    // NOTE(gh) Do u(n+1) = u(n) - (density/dt)*divergence(P)
    f32 c = (density/(dt*cell_dim));
    for(i32 z = 0;
            z < cell_count.z;
            ++z)
    {
        for(i32 y = 0;
                y < cell_count.y;
                ++y)
        {
            for(i32 x = 0;
                    x < cell_count.x;
                    ++x)
            {
                // TODO(gh) For now, we assume the solid wall is not moving 
                // later, we need to change this with the velocity of solid wall based on x, y, z direction
                switch(boundary_direction)
                {
                    case BoundaryDirection_x:
                    {
                        MACID macID = get_index_mac_x(x, y, z, cell_count);
                        if(x == 0)
                        {
                            // solid - fluid
                            dest[macID.ID0] = 0;
                        }
                        else if(x == cell_count.x-1)
                        {
                            // fluid - solid
                            dest[macID.ID1] = 0;
                        }
                        else
                        {
                            // We always(and only) set the dest with higher ID
                            dest[macID.ID1] = source[macID.ID1] - c*(pressures[get_index_mac_center(x+1,y,z,cell_count)]-pressures[get_index_mac_center(x,y,z,cell_count)]);
                        }
                    }break;

                    case BoundaryDirection_y:
                    {
                        MACID macID = get_index_mac_y(x, y, z, cell_count);
                        if(y == 0)
                        {
                            /*
                               fluid
                                 |
                               solid
                            */
                            dest[macID.ID0] = 0;
                        }
                        else if(y == cell_count.y-1)
                        {
                            /*
                               solid
                                 |
                               fluid
                            */
                            dest[macID.ID1] = 0;
                        }
                        else
                        {
                            dest[macID.ID1] = source[macID.ID1] - c*(pressures[get_index_mac_center(x,y+1,z,cell_count)]-pressures[get_index_mac_center(x,y,z,cell_count)]);
                        }
                    }break;

                    case BoundaryDirection_z:
                    {
                        MACID macID = get_index_mac_y(x, y, z, cell_count);
                        if(z == 0)
                        {
                            dest[macID.ID0] = 0;
                        }
                        else if(z == cell_count.z-1)
                        {
                            dest[macID.ID1] = 0;
                        }
                        else
                        {
                            // NOTE(gh) u(n+1) = u(n) - (density/dt)*divergence(P)
                            dest[macID.ID1] = source[macID.ID1] - c*(pressures[get_index_mac_center(x,y,z+1,cell_count)]-pressures[get_index_mac_center(x,y,z,cell_count)]);
                        }
                    }break;
                }
            }
        }
    }
}

internal void
advect(f32 *dest, f32 *source, f32 *v_x, f32 *v_y, f32 *v_z, v3i cell_count, f32 cell_dim)
{}






































