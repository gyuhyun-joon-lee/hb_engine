struct vertex_normal_hit
{
    v3 normal_sum;
    i32 hit;
};

// TODO(joon): exclude duplicate vertex normals
internal void
generate_vertex_normals(MemoryArena *permanent_arena , MemoryArena *trasient_arena, raw_mesh *raw_mesh)
{
    assert(!raw_mesh->normals);

    raw_mesh->normal_count = raw_mesh->position_count;
    raw_mesh->normals = push_array(permanent_arena, v3, sizeof(v3)*raw_mesh->normal_count);

    TempMemory mesh_construction_temp_memory = start_temp_memory(trasient_arena, megabytes(16));
    vertex_normal_hit *normal_hits = push_array(&mesh_construction_temp_memory, vertex_normal_hit, raw_mesh->position_count);

#if MEKA_DEBUG
    begin_cycle_counter(generate_vertex_normals);
#endif

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

        v3 normal = cross(v1, v0);
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
        assert(normal_hit->hit);

        v3 result_normal = normalize(normal_hit->normal_sum/(r32)normal_hit->hit);

        raw_mesh->normals[normal_index] = result_normal;
    }

#if MEKA_DEBUG
    end_cycle_counter(generate_vertex_normals);
#endif

    end_temp_memory(&mesh_construction_temp_memory);
}

/*TODO(joon): 
    This routine is slightly optimized using 128bit-wide SIMD vectors, with around ~2x improvements than before
    Improvements can be made with:
    1. better loading by organizing the data more simd-friendly
    2. using larger simd vectors (float32x4x4?)

    Also, this _only_ works for 4 wide lane simd, and does not work for 1(due to how the memory laid out)
*/
internal void
 generate_vertex_normals_simd(MemoryArena *permanent_arena, MemoryArena *trasient_arena, raw_mesh *raw_mesh)
 {
     assert(!raw_mesh->normals);

     raw_mesh->normal_count = raw_mesh->position_count;
     raw_mesh->normals = push_array(permanent_arena, v3, sizeof(v3)*raw_mesh->normal_count);

     TempMemory mesh_construction_temp_memory = start_temp_memory(trasient_arena, megabytes(16), true);

     r32 *normal_sum_x = push_array(&mesh_construction_temp_memory, r32, raw_mesh->position_count);
     r32 *normal_sum_y = push_array(&mesh_construction_temp_memory, r32, raw_mesh->position_count);
     r32 *normal_sum_z = push_array(&mesh_construction_temp_memory, r32, raw_mesh->position_count);

     u32 *normal_hit_counts = push_array(&mesh_construction_temp_memory, u32, raw_mesh->position_count);

     // NOTE(joon): dividing by 12 as each face is consisted of 3 indices, and we are going to use 4-wide(128 bits) SIMD
     //u32 index_count_div_12 = face_count / 12;
     u32 index_count_mod_12 = raw_mesh->index_count % 12;
     u32 simd_end_index = raw_mesh->index_count - index_count_mod_12;

 #define u32_from_slot(vector, slot) (((u32 *)&vector)[slot])
 #define r32_from_slot(vector, slot) (((r32 *)&vector)[slot])

 #if MEKA_DEBUG
     begin_cycle_counter(generate_vertex_normals);
 #endif
     // TODO(joon) : If we decide to lose the precision while calculating the normals,
     // might be able to use float16, which doubles the faces that we can process at once
     for(u32 i = 0;
             i < simd_end_index;
             i += 12) // considering we are using 128 bit wide vectors
     {


         // Load 12 indices
         // TODO(joon): timed this and for now it's not worth doing so(and increase the code complexity), 
         // unless we change the index to be 16-bit?
         // i00, i01 i02 i10
         uint32x4_t ia = vld1q_u32(raw_mesh->indices + i);
         // i11, i12 i20 i21
         uint32x4_t ib = vld1q_u32(raw_mesh->indices + i + 4);
         // i22, i30 i31 i32
         uint32x4_t ic = vld1q_u32(raw_mesh->indices + i + 8);

         v3 v00 = raw_mesh->positions[u32_from_slot(ia, 0)]; 
         v3 v01 = raw_mesh->positions[u32_from_slot(ia, 1)]; 
         v3 v02 = raw_mesh->positions[u32_from_slot(ia, 2)];

         v3 v10 = raw_mesh->positions[u32_from_slot(ia, 3)]; 
         v3 v11 = raw_mesh->positions[u32_from_slot(ib, 0)]; 
         v3 v12 = raw_mesh->positions[u32_from_slot(ib, 1)];

         v3 v20 = raw_mesh->positions[u32_from_slot(ib, 2)]; 
         v3 v21 = raw_mesh->positions[u32_from_slot(ib, 3)]; 
         v3 v22 = raw_mesh->positions[u32_from_slot(ic, 0)];

         v3 v30 = raw_mesh->positions[u32_from_slot(ic, 1)]; 
         v3 v31 = raw_mesh->positions[u32_from_slot(ic, 2)]; 
         v3 v32 = raw_mesh->positions[u32_from_slot(ic, 3)];

         float32x4_t v0_x = {v00.x, v10.x, v20.x, v30.x};
         float32x4_t v0_y = {v00.y, v10.y, v20.y, v30.y};
         float32x4_t v0_z = {v00.z, v10.z, v20.z, v30.z};

         float32x4_t v1_x = {v01.x, v11.x, v21.x, v31.x};
         float32x4_t v1_y = {v01.y, v11.y, v21.y, v31.y};
         float32x4_t v1_z = {v01.z, v11.z, v21.z, v31.z};

         float32x4_t v2_x = {v02.x, v12.x, v22.x, v32.x};
         float32x4_t v2_y = {v02.y, v12.y, v22.y, v32.y};
         float32x4_t v2_z = {v02.z, v12.z, v22.z, v32.z};

         // NOTE(joon): e0 = v1 - v0, e1 = v2 - v0, normal = cross(e0, e1);

         float32x4_t e0_x_4 = vsubq_f32(v1_x, v0_x);
         float32x4_t e0_y_4 = vsubq_f32(v1_y, v0_y);
         float32x4_t e0_z_4 = vsubq_f32(v1_z, v0_z);

         float32x4_t e1_x_4 = vsubq_f32(v2_x, v0_x);
         float32x4_t e1_y_4 = vsubq_f32(v2_y, v0_y);
         float32x4_t e1_z_4 = vsubq_f32(v2_z, v0_z);

         // x,     y,     z 
         // y0*z1, z0*x1, x0*y1
         float32x4_t cross_left_x = vmulq_f32(e0_y_4, e1_z_4);
         float32x4_t cross_left_y = vmulq_f32(e0_z_4, e1_x_4);
         float32x4_t cross_left_z = vmulq_f32(e0_x_4, e1_y_4);

         // x,     y,     z 
         // z0*y1, x0*z1, y0*x1
         float32x4_t cross_right_x = vmulq_f32(e1_y_4, e0_z_4);
         float32x4_t cross_right_y = vmulq_f32(e0_x_4, e1_z_4);
         float32x4_t cross_right_z = vmulq_f32(e0_y_4, e1_x_4);

         float32x4_t cross_result_x = vsubq_f32(cross_left_x, cross_right_x);
         float32x4_t cross_result_y = vsubq_f32(cross_left_y, cross_right_y);
         float32x4_t cross_result_z = vsubq_f32(cross_left_z, cross_right_z);

         // NOTE(joon): now we have a data that looks like : xxxx yyyy zzzz

         // ia : i00, i01 i02 i10
         // ib : i11, i12 i20 i21
         // ic : i22, i30 i31 i32

         normal_hit_counts[u32_from_slot(ia, 0)]++;
         normal_hit_counts[u32_from_slot(ia, 1)]++;
         normal_hit_counts[u32_from_slot(ia, 2)]++;

         normal_hit_counts[u32_from_slot(ia, 3)]++;
         normal_hit_counts[u32_from_slot(ib, 0)]++;
         normal_hit_counts[u32_from_slot(ib, 1)]++;

         normal_hit_counts[u32_from_slot(ib, 2)]++;
         normal_hit_counts[u32_from_slot(ib, 3)]++;
         normal_hit_counts[u32_from_slot(ic, 0)]++;

         normal_hit_counts[u32_from_slot(ic, 1)]++;
         normal_hit_counts[u32_from_slot(ic, 2)]++;
         normal_hit_counts[u32_from_slot(ic, 3)]++;

         normal_sum_x[u32_from_slot(ia, 0)] += r32_from_slot(cross_result_x, 0);
         normal_sum_x[u32_from_slot(ia, 1)] += r32_from_slot(cross_result_x, 0);
         normal_sum_x[u32_from_slot(ia, 2)] += r32_from_slot(cross_result_x, 0);
         normal_sum_y[u32_from_slot(ia, 0)] += r32_from_slot(cross_result_y, 0);
         normal_sum_y[u32_from_slot(ia, 1)] += r32_from_slot(cross_result_y, 0);
         normal_sum_y[u32_from_slot(ia, 2)] += r32_from_slot(cross_result_y, 0);
         normal_sum_z[u32_from_slot(ia, 0)] += r32_from_slot(cross_result_z, 0);
         normal_sum_z[u32_from_slot(ia, 1)] += r32_from_slot(cross_result_z, 0);
         normal_sum_z[u32_from_slot(ia, 2)] += r32_from_slot(cross_result_z, 0);

         normal_sum_x[u32_from_slot(ia, 3)] += r32_from_slot(cross_result_x, 1);
         normal_sum_x[u32_from_slot(ib, 0)] += r32_from_slot(cross_result_x, 1);
         normal_sum_x[u32_from_slot(ib, 1)] += r32_from_slot(cross_result_x, 1);
         normal_sum_y[u32_from_slot(ia, 3)] += r32_from_slot(cross_result_y, 1);
         normal_sum_y[u32_from_slot(ib, 0)] += r32_from_slot(cross_result_y, 1);
         normal_sum_y[u32_from_slot(ib, 1)] += r32_from_slot(cross_result_y, 1);
         normal_sum_z[u32_from_slot(ia, 3)] += r32_from_slot(cross_result_z, 1);
         normal_sum_z[u32_from_slot(ib, 0)] += r32_from_slot(cross_result_z, 1);
         normal_sum_z[u32_from_slot(ib, 1)] += r32_from_slot(cross_result_z, 1);

         normal_sum_x[u32_from_slot(ib, 2)] += r32_from_slot(cross_result_x, 2);
         normal_sum_x[u32_from_slot(ib, 3)] += r32_from_slot(cross_result_x, 2);
         normal_sum_x[u32_from_slot(ic, 0)] += r32_from_slot(cross_result_x, 2);
         normal_sum_y[u32_from_slot(ib, 2)] += r32_from_slot(cross_result_y, 2);
         normal_sum_y[u32_from_slot(ib, 3)] += r32_from_slot(cross_result_y, 2);
         normal_sum_y[u32_from_slot(ic, 0)] += r32_from_slot(cross_result_y, 2);
         normal_sum_z[u32_from_slot(ib, 2)] += r32_from_slot(cross_result_z, 2);
         normal_sum_z[u32_from_slot(ib, 3)] += r32_from_slot(cross_result_z, 2);
         normal_sum_z[u32_from_slot(ic, 0)] += r32_from_slot(cross_result_z, 2);

         normal_sum_x[u32_from_slot(ic, 1)] += r32_from_slot(cross_result_x, 3);
         normal_sum_x[u32_from_slot(ic, 2)] += r32_from_slot(cross_result_x, 3);
         normal_sum_x[u32_from_slot(ic, 3)] += r32_from_slot(cross_result_x, 3);
         normal_sum_y[u32_from_slot(ic, 1)] += r32_from_slot(cross_result_y, 3);
         normal_sum_y[u32_from_slot(ic, 2)] += r32_from_slot(cross_result_y, 3);
         normal_sum_y[u32_from_slot(ic, 3)] += r32_from_slot(cross_result_y, 3);
         normal_sum_z[u32_from_slot(ic, 1)] += r32_from_slot(cross_result_z, 3);
         normal_sum_z[u32_from_slot(ic, 2)] += r32_from_slot(cross_result_z, 3);
         normal_sum_z[u32_from_slot(ic, 3)] += r32_from_slot(cross_result_z, 3);

     }

     // NOTE(joon): Get the rest of the normals, that cannot be processed in vector-wide fassion
     for(u32 i = simd_end_index;
             i < raw_mesh->index_count;
             i += 3)
     {
         u32 i0 = raw_mesh->indices[i];
         u32 i1 = raw_mesh->indices[i+1];
         u32 i2 = raw_mesh->indices[i+2];

         v3 v0 = raw_mesh->positions[i0] - raw_mesh->positions[i1];
         v3 v1 = raw_mesh->positions[i2] - raw_mesh->positions[i1];

         v3 normal = cross(v1, v0);

         normal_sum_x[i0] += normal.x;
         normal_sum_y[i0] += normal.y;
         normal_sum_z[i0] += normal.z;

         normal_sum_x[i1] += normal.x;
         normal_sum_y[i1] += normal.y;
         normal_sum_z[i1] += normal.z;

         normal_sum_x[i2] += normal.x;
         normal_sum_y[i2] += normal.y;
         normal_sum_z[i2] += normal.z;

         normal_hit_counts[i0]++;
         normal_hit_counts[i1]++;
         normal_hit_counts[i2]++;
     }


     u32 normal_count_mod_4 = raw_mesh->normal_count % 4;
     u32 simd_normal_end_index = raw_mesh->normal_count - normal_count_mod_4;

     for(u32 i = 0;
             i  < simd_normal_end_index;
             i += 4)
     {
         //assert(normal_hit_counts[normal_index]);

         float32x4_t normal_sum_x_128 = vld1q_f32(normal_sum_x + i);
         float32x4_t normal_sum_y_128 = vld1q_f32(normal_sum_y + i);
         float32x4_t normal_sum_z_128 = vld1q_f32(normal_sum_z + i);
         // TODO(joon): test convert vs reinterpret
         float32x4_t normal_hit_counts_128 = vcvtq_f32_u32(vld1q_u32(normal_hit_counts + i));

         // TODO(joon): div vs load one over
         float32x4_t unnormalized_result_x_128 = vdivq_f32(normal_sum_x_128, normal_hit_counts_128);
         float32x4_t unnormalized_result_y_128 = vdivq_f32(normal_sum_y_128, normal_hit_counts_128);
         float32x4_t unnormalized_result_z_128 = vdivq_f32(normal_sum_z_128, normal_hit_counts_128);

         // TODO(joon): normalizing the simd vector might come in handy, pull this out into a seperate function?
         float32x4_t dot_128 = vaddq_f32(vaddq_f32(vmulq_f32(unnormalized_result_x_128, unnormalized_result_x_128), vmulq_f32(unnormalized_result_y_128, unnormalized_result_y_128)), 
                                         vmulq_f32(unnormalized_result_z_128, unnormalized_result_z_128));
         float32x4_t length_128 = vsqrtq_f32(dot_128);

         float32x4_t normalized_result_x_128 = vdivq_f32(unnormalized_result_x_128, length_128);
         float32x4_t normalized_result_y_128 = vdivq_f32(unnormalized_result_y_128, length_128);
         float32x4_t normalized_result_z_128 = vdivq_f32(unnormalized_result_z_128, length_128);

         raw_mesh->normals[i+0] = v3_(r32_from_slot(normalized_result_x_128, 0), 
                                     r32_from_slot(normalized_result_y_128, 0), 
                                     r32_from_slot(normalized_result_z_128, 0));
         raw_mesh->normals[i+1] = v3_(r32_from_slot(normalized_result_x_128, 1), 
                                     r32_from_slot(normalized_result_y_128, 1), 
                                     r32_from_slot(normalized_result_z_128, 1));
         raw_mesh->normals[i+2] = v3_(r32_from_slot(normalized_result_x_128, 2), 
                                     r32_from_slot(normalized_result_y_128, 2), 
                                     r32_from_slot(normalized_result_z_128, 2));
         raw_mesh->normals[i+3] = v3_(r32_from_slot(normalized_result_x_128, 3), 
                                     r32_from_slot(normalized_result_y_128, 3), 
                                     r32_from_slot(normalized_result_z_128, 3));
     }

     // NOTE(joon): calculate the remaining normals
     for(u32 normal_index = simd_normal_end_index;
             normal_index < raw_mesh->normal_count;
             normal_index++)
     {
         v3 normal_sum = v3_(normal_sum_x[normal_index], normal_sum_y[normal_index], normal_sum_z[normal_index]);
         u32 normal_hit_count = normal_hit_counts[normal_index];
         assert(normal_hit_count);

         v3 result_normal = normalize(normal_sum/(r32)normal_hit_count);

         raw_mesh->normals[normal_index] = result_normal;
     }

#if MEKA_DEBUG
     end_cycle_counter(generate_vertex_normals);
#endif

     end_temp_memory(&mesh_construction_temp_memory);
}


internal void
push_voxel(RenderGroup *render_group, v3 p, u32 color)
{
    RenderEntryVoxel *entry = (RenderEntryVoxel *)(render_group->render_push_buffer + render_group->render_push_buffer_used);
    render_group->render_push_buffer_used += sizeof(RenderEntryVoxel);
    assert(render_group->render_push_buffer_used <= render_group->render_push_buffer_max_size);

    entry->header.type = render_entry_type_voxel;
    entry->p = p;
    entry->color = color;
}













