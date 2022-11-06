#ifndef HB_FLUID_H
#define HB_FLUID_H

// NOTE(gh) Initial state for all of the elements is 0
struct FluidCube
{
    v3 left_bottom_p;

    // Determines how resistent the fluid is to flow. High number means the fluid is very resistent.
    f32 viscosity; 

    v3u cell_count; // includes the boundary
    f32 cell_dim; // each cell is a uniform cube
    u32 total_cell_count;
    u32 stride; // sizeof(f32)*total_cell_count

    // 2*total_cell_count for double buffering, except the pressure
    // TODO(gh) This is just screaming for optimization using SIMD ><
    f32 *v_x;
    f32 *v_y;
    f32 *v_z;
    f32 *densities;

    f32 *pressures; // Used implicitly by the projection
    f32 *temp_buffer; // Used for forward advection
     
    f32 *v_x_dest;
    f32 *v_x_source;
    f32 *v_y_dest;
    f32 *v_y_source;
    f32 *v_z_dest;
    f32 *v_z_source;
    f32 *density_dest;
    f32 *density_source;
};

enum ElementTypeForBoundary
{
    // These values require the boundary to be an exact negative value of the neighbor.
    // For example, v0jk.x = -v1jk.x, vi0k.y = -vi1k.y, and so one.
    // This counteracts the fluid from going outside the boundary.
    ElementTypeForBoundary_NegateX, // should negate in xy plane(i.e z value)
    ElementTypeForBoundary_NegateY,
    ElementTypeForBoundary_NegateZ,

    // For these values, we are setting the same value as the neighbor.
    // For example, d0jk = d1jk
    ElementTypeForBoundary_Continuous,
};

// NOTE(gh) This is fluid cube using MAC grid, which is a grid that stores different quantities
// in different locations. For example, we store pressure in the center of the grid, 
// while storing velocities on the edge(or center of the face in 3D) of the grid.
// (i.e store u on vertical edges, and v on horizontal edges in 2D)
struct FluidCubeMAC
{
    v3 min;
    v3 max;

    v3i cell_count; // MAC does not store boundaries, we will explicitly handle the boundaries.
    f32 cell_dim; // each cell is a uniform cube

    i32 total_x_count; // z*y*(x+1)
    i32 total_y_count; // z*(y+1)*x
    i32 total_z_count; // (z+1)*y*x
    i32 total_center_count; // x*y*z

    f32 *v_x; // count : 2*z*y*(x+1)
    f32 *v_y; // count : 2*z*(y+1)*x
    f32 *v_z; // count : 2*(z+1)*y*x

    f32 *densities; // count : x*y*z

    f32 *pressure_x; // Used implicitly by the projection
    f32 *temp_buffer_x; // Used for forward advection
    f32 *pressure_y; // Used implicitly by the projection
    f32 *temp_buffer_y; // Used for forward advection
    f32 *pressure_z; // Used implicitly by the projection
    f32 *temp_buffer_z; // Used for forward advection

    f32 *v_x_dest;
    f32 *v_x_source;
    f32 *v_y_dest;
    f32 *v_y_source;
    f32 *v_z_dest;
    f32 *v_z_source;
    f32 *density_dest;
    f32 *density_source;

    // We will also not store the viscosity, and depend on numerical diffusion due to forward euler
};

// NOTE(gh) Also used for enforcing boundary
// i.e if FluidQuantityType_x, pressure will be adjusted with a 'ghost' pressure for the left and right walls
enum FluidQuantityType
{
    FluidQuantityType_Center, // density, temperature... 
    FluidQuantityType_x, 
    FluidQuantityType_y, 
    FluidQuantityType_z,
};

#endif
