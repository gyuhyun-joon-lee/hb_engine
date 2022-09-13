#ifndef HB_VOXEL_H
#define HB_VOXEL_H

struct Voxel
{
    b32 is_alive;

    // NOTE(gh) position inside the voxel grid
    v3u p;

    f32 move_cooldown;
};

struct VoxelWorld
{
    v3u dim; // in voxel

    // TODO(gh) this can only take 4x4x4 (3D) or 8x8(2D) voxels
    u64 voxel_bits;// voxels, represented in bits(0 = empty, 1 = filled)
    Voxel voxels[64];
    u32 voxel_count;

    v3 voxel_dim;
};

#endif
