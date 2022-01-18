#ifndef MEKA_VOXEL_H
#define MEKA_VOXEL_H

// TODO(joon) use memory arena push functions to allocate the nodes & get the acutal size for free
// NOTE(joon) this chunk can store whatever size of the voxel.
// NOTE(joon) we will go with 8*8*8 voxels per chunk
// NOTE(joon) accessed through hash table
struct voxel_chunk_hash
{
    // TODO(joon) Do we want these values to be i32?
    u32 x;
    u32 y;
    u32 z;

    u32 first_node_offset; // only the chunk hash has this value, as 24 bit is not enough to represent the whole world
};

struct material
{
    u32 color;
    // TODO(joon) can also add texture ID, reflectivity...
};

struct voxel_chunk
{
    u32 x;
    u32 y;
    u32 z;

};

#define voxel_pos_x_mask 0b10101010
#define voxel_neg_x_mask 0b01010101
#define voxel_pos_y_mask 0b11001100
#define voxel_neg_y_mask 0b00110011
#define voxel_pos_z_mask 0b11110000
#define voxel_neg_z_mask 0b00001111

#define voxel_p_000 0b00000001
#define voxel_p_100 0b00000010
#define voxel_p_010 0b00000100
#define voxel_p_110 0b00001000

#define voxel_p_001 0b00010000
#define voxel_p_101 0b00100000
#define voxel_p_011 0b01000000
#define voxel_p_111 0b10000000
/*
   NOTE(joon) voxel bit mask
   x y z 

   0 0 0  0000 0001 
   1 0 0  0000 0010
   0 1 0  0000 0100
   1 1 0  0000 1000

   0 0 1  0001 0000   
   1 0 1  0010 0000
   0 1 1  0100 0000
   1 1 1  1000 0000
*/

#endif
