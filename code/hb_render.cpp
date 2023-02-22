/*
 * Written by Gyuhyun Lee
 */

#include "hb_render.h"

internal Camera
init_fps_camera(v3 p, f32 focal_length, f32 fov_in_degree, f32 near, f32 far)
{
    Camera result = {};

    result.p = p;
    result.focal_length = focal_length;

    result.orientation = Quat(1, V3()); // angle == 0

    result.fov = degree_to_radian(fov_in_degree);
    result.near = near;
    result.far = far;

    return result;
}

internal CircleCamera
init_circle_camera(v3 p, v3 lookat_p, f32 distance_from_axis, f32 fov_in_degree, f32 near, f32 far)
{
    CircleCamera result = {};

    result.p = p;
    // TODO(gh) we are not allowing arbitrary point for this type of camera, for now.
    assert(p.x == 0.0f && p.y == 0);
    result.lookat_p = lookat_p;
    result.distance_from_axis = distance_from_axis;
    result.rad = 0;

    result.fov = degree_to_radian(fov_in_degree);
    result.near = near;
    result.far = far;

    return result;
}

/* 
   NOTE(gh) Rotation matrix
   Let's say that we have a tranform matrix that looks like this
   |xa ya za|
   |xb yb zb|
   |xc yc zc|

   This means that whenever we multiply a point with this, we are projecting the point into a coordinate system
   with three axes : [xa ya za], [xb yb zb], [xc yc zc] by taking a dot product.
   Common example for this would be world to camera transform matrix.

   Let's now setup the matrix in the other way by transposing the matrix above. 
   |xa xb xc|
   |ya yb yc|
   |za zb zc|
   Whenever we multiply a point by this, the result would be p.x*[xa ya za] + p.y*[xb yb zb] + p.z*[xc yc zc]
   Common example for this would be camera to world matrix, where we have a point in camera space and 
   we want to get what the p would be in world coordinates 

   As you can see, two matrix is doing the exact opposite thing, which means they are inverse matrix to each other.
   This make sense, as the rotation matrix is an orthogonal matrix(both rows and the columns are unit vectors),
   so that the inverse = transpose
*/

// NOTE(gh) To move the world space position into camera space,
// 1. We first translate the position so that the origin of the camera space
// in world coordinate is (0, 0, 0). We can achieve this by  simply subtracing the camera position
// from the world position.
// 2. We then project the translated position into the 3 camera axes. This is done simply by using a dot product.
//
// To pack this into a single matrix, we need to multiply the translation by the camera axis matrix,
// because otherwise it will be projection first and then translation -> which is completely different from
// doing the translation and the projection.
// TODO(gh) Make this to use m3x4 matrix instead of m4x4
internal m4x4 
camera_transform(v3 camera_p, v3 camera_x_axis, v3 camera_y_axis, v3 camera_z_axis)
{
    TIMED_BLOCK();
    m4x4 result = {};


    return result;
}

#if 1
internal v3
get_camera_lookat(Camera *camera)
{
    v3 result = orientation_quat_to_m3x3(camera->orientation) * V3(0, 0, -1); 

    return result;
}

internal v3
get_camera_right(Camera *camera)
{
    v3 result = orientation_quat_to_m3x3(camera->orientation) * V3(1, 0, 0);

    return result;
}

internal v3
get_camera_up(Camera *camera)
{
    v3 result = orientation_quat_to_m3x3(camera->orientation) * V3(0, 1, 0);
    return result;
}
#endif

internal m4x4
camera_transform(Camera *camera)
{
    // orientation quaternion should always be a unit quaternion
    assert(compare_with_epsilon(length_square(camera->orientation), 1.0f));

    m3x3 orientation_matrix = orientation_quat_to_m3x3(camera->orientation);

    // TODO(gh) Since the vectors have so many 0s, we can avoid the matrix-vector
    // multiplication
    // We assume that the initial coordinates of the camera matches with the 
    // world coordinates
    v3 camera_x_axis = orientation_matrix*V3(1, 0, 0);
    v3 camera_y_axis = orientation_matrix*V3(0, 1, 0);
    v3 camera_z_axis = orientation_matrix*V3(0, 0, 1);

    // NOTE(gh) to pack the rotation & translation into one matrix(with an order of translation and the rotation),
    // we need to first multiply the translation by the rotation matrix
    v3 multiplied_translation = V3(dot(camera_x_axis, -camera->p), 
                                    dot(camera_y_axis, -camera->p),
                                    dot(camera_z_axis, -camera->p));

    m4x4 result = M4x4();
    result.rows[0] = V4(camera_x_axis, multiplied_translation.x);
    result.rows[1] = V4(camera_y_axis, multiplied_translation.y);
    result.rows[2] = V4(camera_z_axis, multiplied_translation.z);
    // NOTE(gh) Dont need to touch the w part, view matrix doesn't produce homogeneous coordinates
    // This is why we could also use 3x4 matrix here, without the 4th row
    result.rows[3] = V4(0.0f, 0.0f, 0.0f, 1.0f); 

    return result;
}

// NOTE(gh) persepctive projection matrix for (-1, -1, 0) to (1, 1, 1) NDC like Metal
/*
    Little tip in how we get the persepctive projection matrix
    Think as 2D plane(x and z OR y and z), use triangle similarity to get projected Xp and Yp
    For Z, as x and y don't have any effect on Z, we can say (A * Ze + b) / -Ze = Zp (diving by -Ze because homogeneous coords)

    Homogenous coordinates are _required_ because otherwise after the projection 
    we would lost the depth(z ranging from near to far) information. 

    -n should produce 0 or -1 value(based on what NDC system we use), 
    while -f should produce 1.
*/
inline m4x4
perspective_projection_near_is_01(f32 fov, f32 n, f32 f, f32 width_over_height)
{
    assert(fov < 180);

    f32 half_near_plane_width = n*tanf(0.5f*fov)*0.5f;
    f32 half_near_plane_height = half_near_plane_width / width_over_height;

    m4x4 result = {};

    // TODO(gh) Even though the resulting coordinates should be same, 
    // it seems like in Metal w value should be positive.
    // Maybe that's how they are doing the frustum culling..?
    result.rows[0] = V4(n / half_near_plane_width, 0, 0, 0);
    result.rows[1] = V4(0, n / half_near_plane_height, 0, 0);
    result.rows[2] = V4(0, 0, f/(n-f), (n*f)/(n-f)); // X and Y values don't effect the z value
    result.rows[3] = V4(0, 0, -1, 0); // As Xp and Yp are dependant to Z value, this is the only way to divide Xp and Yp by -Ze

    return result;
}


// TODO(gh) This operation is quite expensive, find out to optimize it
internal void
get_camera_frustum(Camera *camera, CameraFrustum *frustum, f32 width_over_height)
{
    v3 camera_dir = get_camera_lookat(camera);
    v3 camera_right = get_camera_right(camera);
    v3 camera_up = normalize(cross(camera_right, camera_dir));

    v3 near_plane_center = camera->p + camera->near * camera_dir;
    v3 far_plane_center = camera->p + camera->far * camera_dir;

    f32 half_near_plane_width = camera->near*tanf(0.5f*camera->fov)*0.5f;
    f32 half_near_plane_height = half_near_plane_width / width_over_height;
    v3 half_near_plane_right = half_near_plane_width * camera_right;
    v3 half_near_plane_up = half_near_plane_height * camera_up;

    f32 half_far_plane_width = camera->far*tanf(0.5f*camera->fov)*0.5f;
    f32 half_far_plane_height = half_far_plane_width / width_over_height;
    v3 half_far_plane_right = half_far_plane_width * camera_right;
    v3 half_far_plane_up = half_far_plane_height * camera_up;

    // morten z order
    frustum->near[0] = near_plane_center - half_near_plane_right + half_near_plane_up;
    frustum->near[1] = near_plane_center + half_near_plane_right + half_near_plane_up;
    frustum->near[2] = near_plane_center - half_near_plane_right - half_near_plane_up;
    frustum->near[3] = near_plane_center + half_near_plane_right - half_near_plane_up;

    frustum->far[0] = far_plane_center - half_far_plane_right + half_far_plane_up;
    frustum->far[1] = far_plane_center + half_far_plane_right + half_far_plane_up;
    frustum->far[2] = far_plane_center - half_far_plane_right - half_far_plane_up;
    frustum->far[3] = far_plane_center + half_far_plane_right - half_far_plane_up;
}

internal m4x4
rhs_to_lhs(m4x4 m)
{
    m4x4 result = m;
    result.rows[2] *= -1.0f;

    return result;
}

struct ThreadPopulateFloorZData
{
    u32 start_x;
    u32 one_past_end_x;

    u32 start_y;
    u32 one_past_end_y;
    
    v2 grass_grid_min;
    v2 grass_seperation_dim;

    u32 grass_count_x;

    simd_v3 *p0;
    simd_v3 *p1;
    simd_v3 *p2;
    u32 simd_vertex_count;
    v3 mesh_offset_p;

    void *floor_z_buffer;
};

internal
THREAD_WORK_CALLBACK(thread_optimized_raycast_straight_down_z_to_non_overlapping_mesh)
{
    ThreadPopulateFloorZData *d = (ThreadPopulateFloorZData *)data;

    for(u32 y = d->start_y;
            y < d->one_past_end_y;
            ++y)
    {
        for(u32 x = d->start_x;
                x < d->one_past_end_x;
                ++x)
        {
            // Using the z value that is high enough that no floor can be higher than this
            v3 ray_origin = V3(d->grass_grid_min + hadamard(V2(x, y), d->grass_seperation_dim), 10000.0f);

            f32 z = optimized_raycast_straight_down_z_to_non_overlapping_mesh(ray_origin, 
                        d->p0, d->p1, d->p2, d->simd_vertex_count,
                        d->mesh_offset_p).z;

            *((f32 *)d->floor_z_buffer + y*d->grass_count_x + x) = z;
        }
    }
}

#if 0
// TODO(gh) Later we would want to minimize passing the platform buffer here
// TODO(gh) pass center & dim, instead of min & max?
internal void
init_grass_grid(ThreadWorkQueue *general_work_queue, ThreadWorkQueue *gpu_work_queue, MemoryArena *arena, Entity *floor,  
                GrassGrid *grass_grid, u32 grass_count_x, u32 grass_count_y, v2 min, v2 max)
{
    grass_grid->grass_count_x = grass_count_x;
    grass_grid->grass_count_y = grass_count_y;
    grass_grid->min = min;
    grass_grid->max = max;

    u32 total_grass_count = grass_grid->grass_count_x * grass_grid->grass_count_y;
    v2 grass_seperation_dim = 
        hadamard(grass_grid->max - grass_grid->min, 
                V2(1.0f/grass_grid->grass_count_x, 1.0f/grass_grid->grass_count_y));

    grass_grid->floor_z_buffer = get_gpu_visible_buffer(gpu_work_queue, sizeof(f32) * total_grass_count);
    u32 work_count_x = 8;
    u32 work_count_y = 8;
    assert(grass_count_x % work_count_x == 0);
    assert(grass_count_y % work_count_y == 0);
    u32 grass_per_work_count_x = grass_count_x/work_count_x;
    u32 grass_per_work_count_y = grass_count_y/work_count_y;
    TempMemory work_memory = start_temp_memory(arena, sizeof(ThreadPopulateFloorZData)*work_count_x*work_count_y);
    ThreadPopulateFloorZData *thread_data = push_array(&work_memory, ThreadPopulateFloorZData, work_count_x*work_count_y);

    assert(floor->index_count%(3*HB_LANE_WIDTH) == 0);
    TempMemory mesh_memory = start_temp_memory(arena, sizeof(simd_v3)*(floor->index_count/4));
    simd_v3 *p0 = push_array(&mesh_memory, simd_v3, floor->index_count/12);
    simd_v3 *p1 = push_array(&mesh_memory, simd_v3, floor->index_count/12);
    simd_v3 *p2 = push_array(&mesh_memory, simd_v3, floor->index_count/12);

    u32 simd_vertex_count = 0;
    for(u32 i = 0;
            i < floor->index_count;
            i += 12)
    {
        u32 i0 = floor->indices[i];
        u32 i1 = floor->indices[i + 1];
        u32 i2 = floor->indices[i + 2];

        u32 i3 = floor->indices[i + 3];
        u32 i4 = floor->indices[i + 4];
        u32 i5 = floor->indices[i + 5];

        u32 i6 = floor->indices[i + 6];
        u32 i7 = floor->indices[i + 7];
        u32 i8 = floor->indices[i + 8];

        u32 i9 = floor->indices[i + 9];
        u32 i10 = floor->indices[i + 10];
        u32 i11 = floor->indices[i + 11];

        p0[simd_vertex_count] = Simd_v3(floor->vertices[i0].p, floor->vertices[i3].p, floor->vertices[i6].p, floor->vertices[i9].p);
        p1[simd_vertex_count] = Simd_v3(floor->vertices[i1].p, floor->vertices[i4].p, floor->vertices[i7].p, floor->vertices[i10].p);
        p2[simd_vertex_count] = Simd_v3(floor->vertices[i2].p, floor->vertices[i5].p, floor->vertices[i8].p, floor->vertices[i11].p);

        simd_vertex_count++;
    }
    assert(simd_vertex_count == (floor->index_count/12));

    for(u32 y = 0;
            y < work_count_y;
            ++y)
    {
        for(u32 x = 0;
                x < work_count_x;
                ++x)
        {
            ThreadPopulateFloorZData *data = thread_data + y*work_count_x + x;
            data->start_x = x * grass_per_work_count_x;
            data->one_past_end_x = data->start_x + grass_per_work_count_x;

            data->start_y = y * grass_per_work_count_y;
            data->one_past_end_y = data->start_y + grass_per_work_count_y;

            data->grass_grid_min = grass_grid->min;
            data->grass_seperation_dim = grass_seperation_dim;

            data->grass_count_x = grass_count_x;
            data->p0 = p0;
            data->p1 = p1;
            data->p2 = p2;
            data->simd_vertex_count = simd_vertex_count;
            data->mesh_offset_p = floor->generic_entity_info.position;
            data->floor_z_buffer = grass_grid->floor_z_buffer.memory;

            general_work_queue->add_thread_work_queue_item(general_work_queue, thread_optimized_raycast_straight_down_z_to_non_overlapping_mesh, 0, data);
        }
    }
    general_work_queue->complete_all_thread_work_queue_items(general_work_queue, true);
    end_temp_memory(&work_memory);
    end_temp_memory(&mesh_memory);

    grass_grid->grass_instance_data_buffer = get_gpu_visible_buffer(gpu_work_queue, sizeof(GrassInstanceData)*total_grass_count);
}
#endif

internal void
init_render_push_buffer(PlatformRenderPushBuffer *render_push_buffer, Camera *render_camera, Camera *game_camera,  
                        GrassGrid *grass_grids, u32 grass_grid_count_x, u32 grass_grid_count_y,
                        v3 clear_color, b32 enable_shadow)
{
    TIMED_BLOCK();
    assert(render_push_buffer->base);

    render_push_buffer->render_camera_view = camera_transform(render_camera);
    render_push_buffer->render_camera_near = render_camera->near;
    render_push_buffer->render_camera_far = render_camera->far;
    render_push_buffer->render_camera_fov = render_camera->fov;
    render_push_buffer->render_camera_p = render_camera->p;

    render_push_buffer->game_camera_view = camera_transform(game_camera);
    render_push_buffer->game_camera_near = game_camera->near;
    render_push_buffer->game_camera_far = game_camera->far;
    render_push_buffer->game_camera_fov = game_camera->fov;
    render_push_buffer->game_camera_p = game_camera->p;

    render_push_buffer->clear_color = clear_color;
    render_push_buffer->grass_grids = grass_grids;
    render_push_buffer->grass_grid_count_x = grass_grid_count_x;
    render_push_buffer->grass_grid_count_y = grass_grid_count_y;

    render_push_buffer->enable_shadow = enable_shadow;
    
    render_push_buffer->transient_buffer_used = 0;

    render_push_buffer->used = 0;
}

internal void *
_push_render_element(PlatformRenderPushBuffer *render_push_buffer, u32 size)
{
    void *entry = (void *)(render_push_buffer->base + render_push_buffer->used);

    render_push_buffer->used += size;
    assert(render_push_buffer->used <= render_push_buffer->total_size);

    return entry;
}
#define push_render_element(render_push_buffer, type) (type *)_push_render_element(render_push_buffer, sizeof(type))

// TODO(gh) do we want to collape this to single line_group or something to save memory(color, type)?
internal void
push_line(PlatformRenderPushBuffer *render_push_buffer, v3 start, v3 end, v3 color)
{
    TIMED_BLOCK();

    RenderEntryLine *entry = push_render_element(render_push_buffer, RenderEntryLine);

    entry->header.type = RenderEntryType_Line;
    entry->header.size = sizeof(*entry);

    entry->start = start;
    entry->end = end;
    entry->color = color;
}

// TODO(gh) This can be confusing, do we want to have just one buffer that holds everything(vertex, index, instance data..?)
internal u32
push_data(void *dst_buffer, u64 *dst_used, u64 dst_size, void *src, u32 src_size)
{
    TIMED_BLOCK();
    u32 original_used = *dst_used;

    void *dst = (u8 *)dst_buffer + original_used;
    memcpy(dst, src, src_size);

    *dst_used += src_size;
    assert(*dst_used <= dst_size);

    return original_used;
}

internal void
push_frustum(PlatformRenderPushBuffer *render_push_buffer, v3 color, 
            v3 *vertices, u32 vertex_count, u32 *indices, u32 index_count)
{
    RenderEntryFrustum *entry = push_render_element(render_push_buffer, RenderEntryFrustum);

    entry->header.type = RenderEntryType_Frustum;
    entry->header.size = sizeof(*entry);

    entry->color = color;

    entry->vertex_buffer_offset = push_data(render_push_buffer->transient_buffer, 
                                            &render_push_buffer->transient_buffer_used, 
                                            render_push_buffer->transient_buffer_size,
                                            vertices, sizeof(vertices[0]) * vertex_count);

    entry->index_buffer_offset = push_data(render_push_buffer->transient_buffer, 
                                            &render_push_buffer->transient_buffer_used, 
                                            render_push_buffer->transient_buffer_size,
                                            indices, sizeof(indices[0]) * index_count);
    entry->index_count = index_count;
}

internal void
push_mesh_pn(PlatformRenderPushBuffer *render_push_buffer, v3 p, v3 dim, v3 color, 
            u32 *mesh_assetID, AssetTag tag, GameAssets *asset)
{
    RenderEntryMeshPN *entry = push_render_element(render_push_buffer, RenderEntryMeshPN);

    entry->header.type = RenderEntryType_MeshPN;
    entry->header.size = sizeof(*entry);

    // TODO(gh) Although necessary, I don't like passing the pointer to the mesh asset ID
    // which is 'inside' the entity. Any way to avoid this?
    MeshAsset *mesh_asset = get_mesh_asset(asset, mesh_assetID, tag);
    entry->vertex_buffer_handle = mesh_asset->vertex_buffer.handle;
    entry->vertex_count = mesh_asset->vertex_count;
    entry->index_buffer_handle = mesh_asset->index_buffer.handle;
    entry->index_count = mesh_asset->index_count;

    entry->p = p;
    entry->dim = dim;
    
    entry->color = color;
}


// TODO(gh) Change this with textured quad? Because we have to have some kind of texture system
// that is visible from the game code someday!
internal void
push_glyph(PlatformRenderPushBuffer *render_push_buffer, FontAsset *font_asset, v3 color, 
            v2 top_left_rel_p_px, u32 codepoint, f32 scale)
{
    RenderEntryGlyph *entry = push_render_element(render_push_buffer, RenderEntryGlyph);

    entry->header.type = RenderEntryType_Glyph;
    entry->header.size = sizeof(*entry);

    u32 glyphID = font_asset->codepoint_to_glyphID_table[codepoint];

    GlyphAsset *glyph_asset = font_asset->glyph_assets + glyphID;
    assert(glyph_asset->texture.handle);
    entry->texture_handle = glyph_asset->texture.handle;
    entry->color = color;

    // TODO(gh) Do we wanna pull this out?
    v2 bottom_left_rel_p_px = V2(top_left_rel_p_px.x, render_push_buffer->window_height - top_left_rel_p_px.y);
    // TODO(gh) also document this sorcery
    v2 min_px = bottom_left_rel_p_px + scale*(V2(0, -font_asset->ascent_from_baseline) + 
                                                V2(0, glyph_asset->y_offset_from_baseline_px));
    
    v2 max_px = min_px + scale*glyph_asset->dim_px;

    entry->min = 2*V2(min_px.x / render_push_buffer->window_width, min_px.y / render_push_buffer->window_height) - V2(1, 1);
    entry->max = 2*V2(max_px.x / render_push_buffer->window_width, max_px.y / render_push_buffer->window_height) - V2(1, 1);

    entry->texcoord_min = V2(0, 0);
    entry->texcoord_max = V2(1, 1);
}

// NOTE(gh) This is NOT the recommended way to draw things, use push_mesh_pn for larger meshes!
internal void
push_arbitrary_mesh(PlatformRenderPushBuffer *render_push_buffer, v3 color, VertexPN *vertices, u32 vertex_count, u32 *indices, u32 index_count)
{
    RenderEntryArbitraryMesh *entry = push_render_element(render_push_buffer, RenderEntryArbitraryMesh);
    entry->header.type = RenderEntryType_ArbitraryMesh;
    entry->header.size = sizeof(*entry);
    entry->header.instance_count = 1;

    entry->color = color;

    entry->vertex_buffer_offset = push_data(render_push_buffer->transient_buffer, 
                                            &render_push_buffer->transient_buffer_used, 
                                            render_push_buffer->transient_buffer_size,
                                            vertices, sizeof(vertices[0]) * vertex_count);

    entry->index_buffer_offset = push_data(render_push_buffer->transient_buffer, 
                                            &render_push_buffer->transient_buffer_used, 
                                            render_push_buffer->transient_buffer_size,
                                            indices, sizeof(indices[0]) * index_count);
    entry->index_count = index_count; 
}

#if 0
internal RenderEntryHeader *
push_mesh_pn(PlatformRenderPushBuffer *render_push_buffer, v3 p, v3 dim, v3 color, 
            u32 *mesh_assetID, AssetTag tag, GameAssets *asset)
{
}
#endif

internal RenderEntryHeader *
start_instanced_rendering(PlatformRenderPushBuffer *render_push_buffer, 
                        void *vertices, u32 single_vertex_size, u32 vertex_count, 
                        void *indices, u32 single_index_size, u32 index_count)
{
    assert(render_push_buffer->recording_instanced_rendering == false);
    render_push_buffer->recording_instanced_rendering = true;

    RenderEntryArbitraryMesh *entry = push_render_element(render_push_buffer, RenderEntryArbitraryMesh);
    entry->header.type = RenderEntryType_ArbitraryMesh;
    entry->header.size = sizeof(*entry);
    entry->header.instance_count = 0;

    entry->vertex_buffer_offset = push_data(render_push_buffer->transient_buffer, 
                                            &render_push_buffer->transient_buffer_used, 
                                            render_push_buffer->transient_buffer_size,
                                            vertices, single_vertex_size * vertex_count);

    entry->index_buffer_offset = push_data(render_push_buffer->transient_buffer, 
                                            &render_push_buffer->transient_buffer_used, 
                                            render_push_buffer->transient_buffer_size,
                                            indices, single_index_size * index_count);
    entry->index_count = index_count;

    entry->instance_buffer_offset = render_push_buffer->transient_buffer_used;

    return (RenderEntryHeader *)entry;
}

internal void
end_instanced_rendering(PlatformRenderPushBuffer *render_push_buffer)
{
    assert(render_push_buffer->recording_instanced_rendering == true);
    render_push_buffer->recording_instanced_rendering = false;
}

internal void
render_all_entities(PlatformRenderPushBuffer *platform_render_push_buffer, 
                    GameState *game_state, GameAssets *game_assets)
{
    b32 draw_particles = true;
    for(u32 entity_index = 0;
        entity_index < game_state->entity_count;
        ++entity_index)
    {
        Entity *entity = game_state->entities + entity_index;
        switch(entity->type)
        {
            case EntityType_Floor:
            {
                // TODO(gh) Don't pass mesh asset ID!!!
                push_mesh_pn(platform_render_push_buffer, 
                          entity->generic_entity_info.position, entity->generic_entity_info.dim, entity->color, 
                          &entity->mesh_assetID,
                          AssetTag_FloorMesh,
                          game_assets);
            }break;

            case EntityType_PBD:
            {
                PBDParticleGroup *group = &entity->particle_group;

                if(draw_particles)
                {
                    for(u32 particle_index = 0;
                            particle_index < group->count;
                            ++particle_index)
                    {
                        PBDParticle *particle = group->particles + particle_index;

                        push_mesh_pn(platform_render_push_buffer, 
                                V3(particle->p), particle->r*V3(1, 1, 1), entity->color, 
                                0,
                                AssetTag_SphereMesh,
                                game_assets);
                    }
                }
                else
                {
                    // TODO(gh) Instead of arbitrary mesh, push mesh pn 
                    // using the rotation matrix(or quaternion) which can be extracted from
                    // the polar decomposition
                }
            }break;
        }
    }
}








