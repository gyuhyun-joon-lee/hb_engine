#include "meka_voxel.h"
#define Hash_Is_Empty U32_Max

#if 0
// NOTE(joon) if the chunk was empty, x value of the chunk hash is set to this value
#define Empty_Hash U32_Max

internal voxel_chunk_hash *
get_voxel_chunk_hash(voxel_chunk_hash *hashes, u32 hash_count, 
                    u32 chunk_x, u32 chunk_y, u32 chunk_z)
{
    // TODO(joon) better hash function
    u32 hash_value = (25*chunk_x + 232*chunk_y + 12 * chunk_z);
    u32 hash_key = hash_value % hash_count;

    // TODO(joon) used to check if we have looped through the whole hash chunk or not, any better way to do this?
    u32 first_index = hash_key;
    u32 search_index = first_index;
    voxel_chunk_hash *found_chunk = 0;
    while(found_chunk == 0)
    {
        voxel_chunk_hash *search = hashes + search_index;
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
            found_chunk->first_child_offset = 0;

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

internal void
insert_voxel(voxel_chunk_hash *hashes, u32 hash_count, 
            u32 chunk_dim, 
            memory_arena *voxel_memory_arena,
            u32 x, u32 y, u32 z) // x, y, z are in world space, in voxel 
{
    u32 chunk_x = x/chunk_dim;
    u32 chunk_y = y/chunk_dim;
    u32 chunk_z = z/chunk_dim;

    voxel_chunk_hash *chunk = get_voxel_chunk_hash(hashes, hash_count, chunk_x, chunk_y, chunk_z);

    u32 *first_node = voxel_memory_arena->base + chunk->first_node_offset;
    u32 child_offset = (*first_node >> 8);
    u8 child_bit = (*first_node & 0xff);
    if(!child_bit)
    {
        // NOTE(joon): This chunk was never accessed, and should be filled
    }
    else
    {
        u32 *end_node = first_node;
        while(1)
        {
            if(end_node->)
        }
    }
}

#if 1
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
                voxel *v = push_struct(memory_to_fill, voxel);
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

// NOTE(joon) inputs are represented in raw voxel space
// TODO(joon) also take account of the the chunk!

struct get_voxel_result
{
    b32 exist;
};

internal void
get_voxel(u8 *start, u32 stride, u32 x, u32 y, u32 z, u32 dim, u32 lod, get_voxel_result *result)
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

                get_voxel_result voxel = {};
                get_voxel(nodes, 0, x, y, z, 256, 0, &voxel);

                assert(voxel.exist == should_exist);
            }
        }
    }
}

internal void
render_all_voxels(id<MTLRenderCommandEncoder> render_encoder, per_object_data *per_object_data, u8 *start, u32 stride, u32 x, u32 y, u32 z, u32 dim, u32 lod)
{
    if(dim == 1)
    {
        v3 center = get_voxel_center_in_meter(x, y, z, v3_(1, 1, 1));

        per_object_data->model = translate_m4(center.x, center.y, center.z)*scale_m4(1, 1, 1);

        [render_encoder setVertexBytes: per_object_data
                        length: sizeof(*per_object_data)
                        atIndex: 2]; 

        [render_encoder drawPrimitives:MTLPrimitiveTypeTriangle
                                vertexStart:0 
                                vertexCount:36];
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

            switch(scan_mask)
            {
                case voxel_p_000:
                {
                    render_all_voxels(render_encoder, per_object_data, next_lod_start, next_stride, x, y, z, dim/2, lod+1);
                }break;

                case voxel_p_100:
                {
                    render_all_voxels(render_encoder, per_object_data, next_lod_start, next_stride, x + half_dim, y, z, dim/2, lod+1);
                }break;

                case voxel_p_010:
                {
                    render_all_voxels(render_encoder, per_object_data, next_lod_start, next_stride, x, y + half_dim, z, dim/2, lod+1);
                }break;

                case voxel_p_110:
                {
                    render_all_voxels(render_encoder, per_object_data, next_lod_start, next_stride, x + half_dim, y + half_dim, z, dim/2, lod+1);
                }break;

                case voxel_p_001:
                {
                    render_all_voxels(render_encoder, per_object_data, next_lod_start, next_stride, x, y, z + half_dim, dim/2, lod+1);
                }break;

                case voxel_p_101:
                {
                    render_all_voxels(render_encoder, per_object_data, next_lod_start, next_stride, x + half_dim, y, z + half_dim, dim/2, lod+1);
                }break;

                case voxel_p_011:
                {
                    render_all_voxels(render_encoder, per_object_data, next_lod_start, next_stride, x, y + half_dim, z + half_dim, dim/2, lod+1);
                }break;

                case voxel_p_111:
                {
                    render_all_voxels(render_encoder, per_object_data, next_lod_start, next_stride, x + half_dim, y + half_dim, z + half_dim, dim/2, lod+1);
                }break;

            }
        }

        scan_mask = scan_mask >> 1;
    }
}

