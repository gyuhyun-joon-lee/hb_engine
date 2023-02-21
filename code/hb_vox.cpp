

/*
   NOTE(gh) Each chunk in vox file has this kind of structure
   -------------------------------------------------------------------------------
    # Bytes  | Type       | Value
    -------------------------------------------------------------------------------
    1x4      | char       | chunk id
    4        | int        | num bytes of chunk content (N)
    4        | int        | num bytes of children chunks (M)

    N        |            | chunk content

    M        |            | children chunks
    -------------------------------------------------------------------------------
*/
internal LoadedVOXResult
load_vox(u8 *file, u32 file_size)
{
    LoadedVOXResult result = {};

    u8 *current = file;
    u8 *end = file + file_size;
    while(current != end)
    {
        if(*current == 'S' &&
            *(current + 1) == 'I' &&
            *(current + 2) == 'Z' &&
            *(current + 3) == 'E')
        {
            i32 *internal_current = (i32 *)(current + 4); 
            i32 chunk_content_size = *(internal_current++);
            i32 children_chunk_content_size = *(internal_current++);
            assert(children_chunk_content_size == 0);

            result.x_count = *(internal_current++);
            result.y_count = *(internal_current++);
            result.z_count = *(internal_current++);
        }
        else if(*current == 'X' &&
                *(current + 1) == 'Y' &&
                *(current + 2) == 'Z' &&
                *(current + 3) == 'I')
        {
            i32 *internal_current = (i32 *)(current + 4); 
            i32 chunk_content_size = *(internal_current++);
            i32 children_chunk_content_size = *(internal_current++);
            assert(children_chunk_content_size == 0);

            result.voxel_count = *(internal_current++);
            result.xs = (u8 *)malloc(result.voxel_count);
            result.ys = (u8 *)malloc(result.voxel_count);
            result.zs = (u8 *)malloc(result.voxel_count);
            for(u32 voxel_index = 0;
                    voxel_index < result.voxel_count;
                    ++voxel_index)
            {
                i32 xyzi = *internal_current++;
                result.xs[voxel_index] = ((xyzi >> 16) & 0xff);
                result.ys[voxel_index] = ((xyzi >> 8) & 0xff);
                result.zs[voxel_index] = ((xyzi >> 0) & 0xff);
            }
        }
        else if(*current == 'P' &&
                *(current + 1) == 'A' &&
                *(current + 2) == 'C' &&
                *(current + 3) == 'K')
        {
            invalid_code_path;
        }

        current++;
    }
    return result;
}

