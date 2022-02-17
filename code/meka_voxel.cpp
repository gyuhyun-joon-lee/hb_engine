#include "meka_voxel.h"
#define Hash_Is_Empty U32_Max

#if 1
// NOTE(joon) if the chunk was empty, x value of the chunk hash is set to this value
#define Empty_Hash U32_Max

internal void
initialize_voxel_world(VoxelWorld *world)
{
    u32 hash_count = array_count(world->chunk_hashes);
    for(u32 hash_index = 0;
            hash_index < hash_count;
            ++hash_index)
    {
        VoxelChunkHash *hash = world->chunk_hashes + hash_index;
        hash->x = Empty_Hash;
        hash->y = Empty_Hash;
        hash->z = Empty_Hash;

        hash->first_node_offset = Empty_Hash; 
    }

    // TODO(joon) this helps when we simulate the voxels, but we can use SVO or DAG when we 'store' the voxels as a map!
    // or, we can just store the leaf nodes sparsely, as the leaf nodes takes the most of the space
    for(u32 i = 0;
            i < 8;
            ++i)
    {
        world->node_count_per_chunk += power((u32)8, (u32)i);
    }

    world->chunk_dim = 256;
    world->lod = 8;
    assert(power((u32)2, (u32)world->lod) == world->chunk_dim);
}

internal VoxelChunkHash *
get_voxel_chunk_hash(VoxelChunkHash *hashes, u32 hash_count, 
                    u32 chunk_x, u32 chunk_y, u32 chunk_z)
{
    // TODO(joon) better hash function
    u32 hash_value = (25*chunk_x + 232*chunk_y + 12 * chunk_z);
    u32 hash_key = hash_value % hash_count;

    // TODO(joon) used to check if we have looped through the whole hash chunk or not, any better way to do this?
    u32 first_index = hash_key;
    u32 search_index = first_index;
    VoxelChunkHash *found_chunk = 0;
    while(found_chunk == 0)
    {
        VoxelChunkHash *search = hashes + search_index;
        if(search->x == chunk_x && search->y == chunk_y && search->z == chunk_z)
        {
            found_chunk = search;
            break;
        }
        else if(search->x == Empty_Hash)
        {
            found_chunk = search;

            found_chunk->x = chunk_x;
            found_chunk->y = chunk_y;
            found_chunk->z = chunk_z;

            break;
        }
        else
        {
            search_index = (search_index + 1) % hash_count;
            assert(search_index != first_index); // TODO(joon) Every hash is full!
        }
    }

    assert(found_chunk);

    return found_chunk;
}

#if 0
struct voxel
{
    u32 x;
    u32 y;
    u32 z;
};


internal void 
get_all_voxels_inside_chunk(u32 *node, temp_memory *memory_to_fill, u32 x, u32 y, u32 z, u32 dim)
{
    u32 offset_to_child = (u32)(*node >> 8);
    u8 child_bit = (u8)(*node & 0xff);

    u32 chunk_half_dim = dim/2;
    u32 *child_nodes = node + offset_to_child;

    for(u32 bit_shift_index = 0;
            bit_shift_index < 8;
            ++bit_shift_index)
    {
        u32 voxel_x = x;
        u32 voxel_y = y;
        u32 voxel_z = z;
        if(child_bit & (1 << bit_shift_index))
        {
            switch(bit_shift_index)
            {
                case 0:
                {
                    //0 0 0  0000 0001 
                }break;
                case 1:
                {
                    //1 0 0  0000 0010
                    voxel_x += dim;
                }break;
                case 2:
                {
                    //0 1 0  0000 0100
                    voxel_y += dim;
                }break;
                case 3:
                {
                    //1 1 0  0000 1000
                    voxel_x += dim;
                    voxel_y += dim;
                }break;
                case 4:
                {
                    //0 0 1  0001 0000   
                    voxel_z += dim;
                }break;
                case 5:
                {
                    //1 0 1  0010 0000
                    voxel_x += dim;
                    voxel_z += dim;
                }break;
                case 6:
                {
                    //0 1 1  0100 0000
                    voxel_y += dim;
                    voxel_z += dim;
                }break;
                case 7:
                {
                    //1 1 1  1000 0000
                    voxel_x += dim;
                    voxel_y += dim;
                    voxel_z += dim;
                }break;
            }

            if(offset_to_child == 0)
            {
                assert(dim == 1);
                v3u *voxel = push_struct(memory_to_fill, v3u);
                v->x = voxel_x;
                v->y = voxel_y;
                v->z = voxel_z;
            }
            else
            {
                u32 *next_node = (u32 *)((u8 *)node + offset_to_child);
                // recursive
                get_all_voxels_inside_chunk(next_node, memory_to_fill, voxel_x, voxel_y, voxel_z, dim/2);
            }

            child_nodes++;
        }
    }
}
#endif
#endif

internal v3
get_voxel_center_in_meter(u32 voxel_x, u32 voxel_y, u32 voxel_z, v3 voxel_dim)
{
    v3 result = {};

    v3 half_dim = 0.5f * voxel_dim;

    result.x = voxel_dim.x * voxel_x + half_dim.x;
    result.y = voxel_dim.y * voxel_y + half_dim.y;
    result.z = voxel_dim.z * voxel_z + half_dim.z;

    return result;
}

struct voxel_chunk_surface_mask
{
    u8 *pos_x_mask;
    u8 *neg_x_mask;

    u8 *pos_y_mask;
    u8 *neg_y_mask;

    u8 *pos_z_mask;
    u8 *neg_z_mask;
};

internal void
insert_voxel(u8 *start, u32 stride, u32 x, u32 y, u32 z, u32 dim, u32 lod)
{
    if(dim == 1)
    {
        return;
    }

    u8 x_mask = 0;
    u8 y_mask = 0;
    u8 z_mask = 0;

    u32 half_dim = dim/2;
    if(z >= half_dim)
    {
        z_mask = voxel_pos_z_mask;
        z -= half_dim;
    }
    else
    {
        z_mask = voxel_neg_z_mask;
    }

    if(x >= half_dim)
    {
        x_mask = voxel_pos_x_mask;
        x -= half_dim;
    }
    else
    {
        x_mask = voxel_neg_x_mask;
    }

    if(y >= half_dim)
    {
        y_mask = voxel_pos_y_mask;
        y -= half_dim;
    }
    else
    {
        y_mask = voxel_neg_y_mask;
    }

    u8 *node = start + stride;

    u32 node_size = 1;

    u8 voxel_mask = x_mask & y_mask & z_mask; 
    *node = *node | voxel_mask;

    u8 *next_lod_start = start + node_size * power((u32)8, (u32)lod);
    u32 set_bit_index = find_most_significant_bit(voxel_mask);
    u32 next_stride = 8 * stride + set_bit_index * node_size;

    insert_voxel(next_lod_start, next_stride, x, y, z, dim/2, lod+1);
}


internal void
allocate_voxel_chunk_from_vox_file(VoxelWorld *world, MemoryArena *arena, load_vox_result vox)
{
    for(u32 voxel_index = 0;
            voxel_index < vox.voxel_count;
            ++voxel_index)
    {
        // TODO(joon) we also need to take account of the chunk pos?
        u8 x = vox.xs[voxel_index];
        u8 y = vox.ys[voxel_index];
        u8 z = vox.zs[voxel_index];

        // TODO(joon) This might take more time, but is necessarily because the order of the voxels are unpredictable
        // because we are keep loading the voxels from the vox file without keeping the chunk information.
        // we can definitely do better here
        VoxelChunkHash *chunk = get_voxel_chunk_hash(world->chunk_hashes, array_count(world->chunk_hashes), x, y, z);

        u8 *nodes = 0;
        if(chunk->first_node_offset == U32_Max)
        {
            u32 prev_used_amount = arena->used;
            nodes = (u8 *)push_size(arena, world->node_count_per_chunk);
            chunk->first_node_offset = prev_used_amount;
        }
        else
        {
            nodes = (u8 *)arena->base + chunk->first_node_offset;
        }

        insert_voxel(nodes, 0, x, y, z, world->chunk_dim, 0);
    }

    //validate_voxels_with_vox_file(loaded_vox, nodes);
}

// NOTE(joon) inputs are represented in raw voxel space
// TODO(joon) also take account of the the chunk!

struct GetVoxelResult
{
    b32 exist;
};

internal void
get_voxel(u8 *start, u32 stride, u32 x, u32 y, u32 z, u32 dim, u32 lod, GetVoxelResult *result)
{
    u32 node_size = 1;

    u8 x_mask = 0;
    u8 y_mask = 0;
    u8 z_mask = 0;

    u32 half_dim = dim/2;
    if(z >= half_dim)
    {
        z_mask = voxel_pos_z_mask;
        z -= half_dim;
    }
    else
    {
        z_mask = voxel_neg_z_mask;
    }

    if(x >= half_dim)
    {
        x_mask = voxel_pos_x_mask;
        x -= half_dim;
    }
    else
    {
        x_mask = voxel_neg_x_mask;
    }

    if(y >= half_dim)
    {
        y_mask = voxel_pos_y_mask;
        y -= half_dim;
    }
    else
    {
        y_mask = voxel_neg_y_mask;
    }

    u8 *node = start + stride;
    u8 voxel_mask = x_mask & y_mask & z_mask; 

    if(*node & voxel_mask)
    {
        if(dim == 2)
        {
            assert(x < 2);
            assert(y < 2);
            assert(z < 2);

            result->exist = true;
            return;
        }

        u8 *next_lod_start = start + node_size * power((u32)8, (u32)lod);
        u32 set_bit_index = find_most_significant_bit(voxel_mask);
        u32 next_stride = 8 * stride + set_bit_index * node_size;

        get_voxel(next_lod_start, next_stride, x, y, z, dim/2, lod+1, result);
    }
    else
    {
        result->exist = false;
        return;
    }
}

// NOTE(joon) 
internal void
validate_voxels_with_vox_file(load_vox_result vox, u8 *nodes)
{
    for(u32 z = 0;
            z < 256;
            ++z)
    {
        for(u32 y = 0;
                y < 256;
                ++y)
        {
            for(u32 x = 0;
                    x < 256;
                    ++x)                       
            {
                b32 should_exist = false;

                for(u32 voxel_index = 0;
                        voxel_index < vox.voxel_count;
                        ++voxel_index)
                {
                    u8 vox_x = vox.xs[voxel_index];
                    u8 vox_y = vox.ys[voxel_index];
                    u8 vox_z = vox.zs[voxel_index];

                    if(x == vox_x && y == vox_y && z == vox_z)
                    {
                        should_exist = true;
                        break;
                    }

                }

                if(x == 0 && y == 0 && z == 2)
                {
                    int breakhere = 0;
                }

                GetVoxelResult voxel = {};
                get_voxel(nodes, 0, x, y, z, 256, 0, &voxel);

                assert(voxel.exist == should_exist);
            }
        }
    }
}

internal void
push_all_voxels(void *push_buffer, u32 *push_buffer_size, u32 total_push_buffer_size, u8 *start, u32 stride, u32 x, u32 y, u32 z, u32 dim, u32 lod)
{
    if(dim == 1)
    {
        v3 center = get_voxel_center_in_meter(x, y, z, V3(1, 1, 1));

        // TODO(joon) : push here!
        return;
    }

    u8 *node = start + stride;
    u32 node_size = 1;

    u32 half_dim = dim/2;

    u8 child_mask = *node;
    u8 scan_mask = 128;
    for(u32 bit_shift_index = 0;
            bit_shift_index < 8;
            ++bit_shift_index)
    {
        if(child_mask & scan_mask)
        {
            u8 *next_lod_start = start + node_size * power((u32)8, (u32)lod);
            u32 next_stride = 8 * stride + bit_shift_index * node_size;

            u32 next_x = x;
            u32 next_y = y;
            u32 next_z = z;

            switch(scan_mask)
            {
                case voxel_p_000:
                {
                }break;

                case voxel_p_100:
                {
                    next_x += half_dim;
                }break;

                case voxel_p_010:
                {
                    next_y += half_dim;
                }break;

                case voxel_p_110:
                {
                    next_x += half_dim;
                    next_y += half_dim;
                }break;

                case voxel_p_001:
                {
                    next_z += half_dim;
                }break;

                case voxel_p_101:
                {
                    next_x += half_dim;
                    next_z += half_dim;
                }break;

                case voxel_p_011:
                {
                    next_y += half_dim;
                    next_z += half_dim;
                }break;

                case voxel_p_111:
                {
                    next_x += half_dim;
                    next_y += half_dim;
                    next_z += half_dim;
                }break;

            }

            //mtl_render_all_voxels(render_encoder, per_object_data, next_lod_start, next_stride, next_x, next_y, next_z, dim/2, lod+1);
        }

        scan_mask = scan_mask >> 1;
    }
}


