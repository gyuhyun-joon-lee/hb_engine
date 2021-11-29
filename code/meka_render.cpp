struct vertex_normal_hit
{
    v3 normal_sum;
    i32 hit;
};

internal void
generate_vertex_normals(memory_arena *permanent_arena , memory_arena *trasient_arena, raw_mesh *raw_mesh)
{
    assert(!raw_mesh->normals);

    raw_mesh->normal_count = raw_mesh->position_count;
    raw_mesh->normals = push_array(permanent_arena, v3, sizeof(v3)*raw_mesh->normal_count);

    temp_memory mesh_construction_temp_memory = start_temp_memory(trasient_arena, megabytes(16));
    vertex_normal_hit *normal_hits = push_array(&mesh_construction_temp_memory, vertex_normal_hit, raw_mesh->position_count);

    for(u32 normal_index = 0;
            normal_index < raw_mesh->normal_count;
            ++normal_index)
    {
        vertex_normal_hit *normal_hit = normal_hits + normal_index;
        *normal_hit = {};
    }

    for(u32 i = 0;
            i < raw_mesh->index_count;
            i += 3)
    {
        u32 i0 = raw_mesh->indices[i];
        u32 i1 = raw_mesh->indices[i+1];
        u32 i2 = raw_mesh->indices[i+2];

        v3 v0 = raw_mesh->positions[i0] - raw_mesh->positions[i1];
        v3 v1 = raw_mesh->positions[i2] - raw_mesh->positions[i1];

        v3 normal = normalize(Cross(v1, v0));

        normal_hits[i0].normal_sum += normal;
        normal_hits[i0].hit++;
        normal_hits[i1].normal_sum += normal;
        normal_hits[i1].hit++;
        normal_hits[i2].normal_sum += normal;
        normal_hits[i2].hit++;
    }

    for(u32 normal_index = 0;
            normal_index < raw_mesh->normal_count;
            ++normal_index)
    {
        vertex_normal_hit *normal_hit = normal_hits + normal_index;
        v3 result_normal = normal_hit->normal_sum/(r32)normal_hit->hit;

        raw_mesh->normals[normal_index] = result_normal;
    }

    end_temp_memory(&mesh_construction_temp_memory);
}
