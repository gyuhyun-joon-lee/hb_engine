#ifndef HB_VOX_H
#define HB_VOX_H

struct LoadedVOXResult
{
    i32 x_count; 
    i32 y_count;
    i32 z_count;

    i32 voxel_count;
    u8 *xs;
    u8 *ys;
    u8 *zs;
};

#endif
