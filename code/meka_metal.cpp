/*
   TODO(joon)
   metal
   - use indirect command buffer for drawing voxels
*/

internal MTLViewport
metal_set_viewport(f32 x, f32 y, f32 width, f32 height, f32 near = 0.0f, f32 far = 1.0f)
{
    MTLViewport result = {};

    result.originX = (f32)x;
    result.originY = (f32)y;
    result.width = (f32)width;
    result.height = (f32)height;
    result.znear = (f32)near;
    result.zfar = (f32)far;

    return result;
}

internal render_mesh
metal_create_render_mesh_from_raw_mesh(id<MTLDevice> device, raw_mesh *raw_mesh, memory_arena *memory_arena)
{
    assert(raw_mesh->normals);
    assert(raw_mesh->position_count == raw_mesh->normal_count);
    // TODO(joon) : Does not allow certain vertex to have different normal index per face
    if(raw_mesh->normal_indices)
    {
        assert(raw_mesh->indices[0] == raw_mesh->normal_indices[0]);
    }

    // TODO(joon) : not super important, but what should be the size of this
    temp_memory mesh_temp_memory = start_temp_memory(memory_arena, megabytes(16));

    render_mesh result = {};
    temp_vertex *vertices = push_array(&mesh_temp_memory, temp_vertex, raw_mesh->position_count);
    for(u32 vertex_index = 0;
            vertex_index < raw_mesh->position_count;
            ++vertex_index)
    {
        temp_vertex *vertex = vertices + vertex_index;

        // TODO(joon): does vf3 = v3 work just fine?
        vertex->p.x = raw_mesh->positions[vertex_index].x;
        vertex->p.y = raw_mesh->positions[vertex_index].y;
        vertex->p.z = raw_mesh->positions[vertex_index].z;

        assert(is_normalized(raw_mesh->normals[vertex_index]));

        vertex->normal.x = raw_mesh->normals[vertex_index].x;
        vertex->normal.y = raw_mesh->normals[vertex_index].y;
        vertex->normal.z = raw_mesh->normals[vertex_index].z;
    }

    // NOTE/joon : MTLResourceStorageModeManaged requires the memory to be explictly synchronized
    u32 vertex_buffer_size = sizeof(temp_vertex)*raw_mesh->position_count;
    result.vertex_buffer = [device newBufferWithBytes: vertices
                                        length: vertex_buffer_size 
                                        options: MTLResourceStorageModeManaged];
    [result.vertex_buffer didModifyRange:NSMakeRange(0, vertex_buffer_size)];

    u32 index_buffer_size = sizeof(raw_mesh->indices[0])*raw_mesh->index_count;
    result.index_buffer = [device newBufferWithBytes: raw_mesh->indices
                                        length: index_buffer_size
                                        options: MTLResourceStorageModeManaged];
    [result.index_buffer didModifyRange:NSMakeRange(0, index_buffer_size)];

    result.index_count = raw_mesh->index_count;

    end_temp_memory(&mesh_temp_memory);

    return result;
}
