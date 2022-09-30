/*
 * Written by Gyuhyun Lee
 */

#include "hb_voxel.h"

internal void
initialize_voxel_world(VoxelWorld *world, u32 dim_x, u32 dim_y, u32 dim_z, v3 voxel_dim)
{
    world->voxel_count = 0;
    world->voxel_bits = 0;

    world->dim.x = dim_x;
    world->dim.y = dim_y;
    world->dim.z = dim_z;

    world->voxel_dim = voxel_dim;
}

internal u32
get_voxel_index(VoxelWorld *world, u32 x, u32 y, u32 z)
{
    assert(x < world->dim.x && y < world->dim.y && z < world->dim.z);
    u32 result = z * world->dim.y * world->dim.x + 
                      y * world->dim.x + 
                      x;

    return result;
}

internal void
add_voxel(VoxelWorld *world, u32 x, u32 y, u32 z)
{
    u32 voxel_index = get_voxel_index(world, x, y, z); 
    assert(x < world->dim.x && y < world->dim.y && z < world->dim.z);

    // NOTE(gh) check whether the position is outside the world
    assert(voxel_index < array_count(world->voxels));

    // NOTE(gh) check whether the grid was already filled
    assert((world->voxel_bits & (1 << voxel_index)) == 0);
    world->voxel_bits |= (1 << voxel_index);  

    Voxel *voxel = world->voxels + world->voxel_count++;
    assert(world->voxel_count <= array_count(world->voxels));

    voxel->is_alive = true;
    voxel->p.x = x;
    voxel->p.y = y;
    voxel->p.z = z;
}



