/*
 * Written by Gyuhyun Lee
 */
#include "hb_asset.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

internal TextureAsset2D
load_texture_asset(ThreadWorkQueue *gpu_work_queue, void *src, i32 width, i32 height, i32 bytes_per_pixel)
{
    TextureAsset2D result = {};

    assert(src);

    // Allocate the space in GPU and get the handle
    ThreadAllocateTexture2DData allocate_texture2D_data = {};
    allocate_texture2D_data.handle_to_populate = &result.handle;
    allocate_texture2D_data.width = width;
    allocate_texture2D_data.height = height;
    allocate_texture2D_data.bytes_per_pixel = bytes_per_pixel;

    // Load the file into texture
    gpu_work_queue->add_thread_work_queue_item(gpu_work_queue, 0, GPUWorkType_AllocateTexture2D, &allocate_texture2D_data);
    gpu_work_queue->complete_all_thread_work_queue_items(gpu_work_queue, false);

    return result;
}


// TODO(gh) Only loads VertexPN vertices
// TODO(gh) Don't love that we need to pass and update the assetID, make this a tag-based search
internal MeshAsset *
get_mesh_asset(GameAssets *asset, ThreadWorkQueue *gpu_work_queue, u32 *assetID, VertexPN *vertices, u32 vertex_count, u32 *indices, u32 index_count)
{
    MeshAsset *result = 0;
    if(*assetID == 0)
    {
        *assetID = asset->populated_mesh_asset++;
        result = asset->mesh_assets + *assetID;
        
        {
            u64 vertex_buffer_size = sizeof(vertices[0])*vertex_count;
            ThreadAllocateBufferData allocate_buffer_data = {};
            allocate_buffer_data.handle_to_populate = &result->vertex_buffer_handle;
            allocate_buffer_data.size_to_allocate = vertex_buffer_size;
            gpu_work_queue->add_thread_work_queue_item(gpu_work_queue, 0, GPUWorkType_AllocateBuffer, &allocate_buffer_data);
            // TODO(gh) Add some sort of fencing instead of waiting?
            gpu_work_queue->complete_all_thread_work_queue_items(gpu_work_queue, false);

            ThreadWriteEntireBufferData write_entire_buffer_data = {};
            write_entire_buffer_data.handle = result->vertex_buffer_handle;
            write_entire_buffer_data.source = vertices;
            write_entire_buffer_data.size_to_write = vertex_buffer_size;

            gpu_work_queue->add_thread_work_queue_item(gpu_work_queue, 0, GPUWorkType_WriteEntireBuffer, &write_entire_buffer_data);
            gpu_work_queue->complete_all_thread_work_queue_items(gpu_work_queue, false);
            result->vertex_count = vertex_count;
        }
         
        {
            u64 index_buffer_size = sizeof(indices[0])*index_count;
            ThreadAllocateBufferData allocate_buffer_data = {};
            allocate_buffer_data.handle_to_populate = &result->index_buffer_handle;
            allocate_buffer_data.size_to_allocate = index_buffer_size;
            gpu_work_queue->add_thread_work_queue_item(gpu_work_queue, 0, GPUWorkType_AllocateBuffer, &allocate_buffer_data);
            gpu_work_queue->complete_all_thread_work_queue_items(gpu_work_queue, false);

            ThreadWriteEntireBufferData write_entire_buffer_data = {};
            write_entire_buffer_data.handle = result->index_buffer_handle;
            write_entire_buffer_data.source = indices;
            write_entire_buffer_data.size_to_write = index_buffer_size;

            gpu_work_queue->add_thread_work_queue_item(gpu_work_queue, 0, GPUWorkType_WriteEntireBuffer, &write_entire_buffer_data);
            gpu_work_queue->complete_all_thread_work_queue_items(gpu_work_queue, false);
            result->index_count = index_count;
        }

    }
    else
    {
        // The asset has already been loaded 
        result = asset->mesh_assets + *assetID;
    }

    assert(result != 0);

    return result;
}

internal void
begin_load_font(LoadFontInfo *load_font_info, FontAsset *font_asset, const char *file_path, PlatformAPI *platform_api, void *device, u32 max_glyph_count, f32 desired_font_height_px)
{
    load_font_info->font_asset = font_asset;
    load_font_info->font_asset->max_glyph_count = max_glyph_count;
    load_font_info->device = device;

    PlatformReadFileResult font_data = platform_api->read_file(file_path);
    load_font_info->desired_font_height_px = desired_font_height_px;

    stbtt_InitFont(&load_font_info->font_info, font_data.memory, 0);
    load_font_info->font_scale = stbtt_ScaleForPixelHeight(&load_font_info->font_info, desired_font_height_px);

    int ascent;
    int descent;
    int line_gap;
    stbtt_GetFontVMetrics(&load_font_info->font_info, &ascent, &descent, &line_gap);

    font_asset->ascent_from_baseline = load_font_info->font_scale * ascent;
    font_asset->descent_from_baseline = -1.0f*load_font_info->font_scale * descent; // stb library gives us negative value, but we want positive value for this
    font_asset->line_gap = load_font_info->font_scale*line_gap;

    // TODO(gh) We should get rid of these mallocs, for sure.
    u32 codepoint_to_glyphID_table_size = sizeof(u32) * MAX_UNICODE_CODEPOINT;
    font_asset->codepoint_to_glyphID_table = (u16 *)malloc(sizeof(u16) * MAX_UNICODE_CODEPOINT);
    zero_memory(font_asset->codepoint_to_glyphID_table, codepoint_to_glyphID_table_size);

    font_asset->glyph_assets = (GlyphAsset *)malloc(sizeof(GlyphAsset) * max_glyph_count);
    font_asset->kerning_advances = (f32 *)malloc(sizeof(f32) * max_glyph_count * max_glyph_count);
}

#if 1 
internal void
end_load_font(LoadFontInfo *load_font_info)
{
    FontAsset *font_asset = load_font_info->font_asset;
    for(u32 i = 0;
            i < font_asset->max_glyph_count;
            ++i)
    {
        for(u32 j = 0;
                j < font_asset->max_glyph_count;
                ++j)
        {
            u32 codepoint0 = font_asset->glyph_assets[i].unicode_codepoint;
            u32 codepoint1 = font_asset->glyph_assets[j].unicode_codepoint;

            f32 kerning = load_font_info->font_scale*stbtt_GetCodepointKernAdvance(&load_font_info->font_info, codepoint0 ,codepoint1);
            font_asset->kerning_advances[i*font_asset->max_glyph_count + j] = kerning;
        }
    }

    load_font_info->font_asset = 0;
}
#endif

internal void
add_glyph_asset(LoadFontInfo *load_font_info, ThreadWorkQueue *gpu_work_queue, u32 unicode_codepoint)
{
    int glyphID_that_doesnt_exist = stbtt_FindGlyphIndex(&load_font_info->font_info, 11111);     
    assert(stbtt_FindGlyphIndex(&load_font_info->font_info, unicode_codepoint)!=glyphID_that_doesnt_exist);

    u16 glyphID = load_font_info->populated_glyph_count++;
    assert(load_font_info->populated_glyph_count <= load_font_info->font_asset->max_glyph_count);
    load_font_info->font_asset->codepoint_to_glyphID_table[unicode_codepoint] = glyphID;

    int x_advance_px;
    int left_bearing_px;
    stbtt_GetCodepointHMetrics(&load_font_info->font_info, unicode_codepoint, &x_advance_px, &left_bearing_px);

    GlyphAsset *glyph_asset = load_font_info->font_asset->glyph_assets + glyphID; 
    glyph_asset->unicode_codepoint = unicode_codepoint;
    glyph_asset->left_bearing_px = load_font_info->font_scale*left_bearing_px;
    glyph_asset->x_advance_px = load_font_info->font_scale*x_advance_px;
     
    int width;
    int height;
    int x_off;
    int y_off;
    unsigned char *bitmap = stbtt_GetCodepointBitmap(&load_font_info->font_info, 
                                                    load_font_info->font_scale, load_font_info->font_scale,
                                                    unicode_codepoint,
                                                    &width, &height, &x_off, &y_off);

    // scale is already baked in
    glyph_asset->dim_px = V2(width, height);
    glyph_asset->x_offset_px = x_off;
    glyph_asset->y_offset_from_baseline_px = -(height + y_off); 

    if(bitmap)
    {
        glyph_asset->texture = load_texture_asset(gpu_work_queue, bitmap, width, height, 1);
        stbtt_FreeBitmap(bitmap, 0);
    }
}

// TODO(gh) 'stream' assets?
// TODO(gh) This is pretty much it when we need the mtldevice for resource allocations,
// but can we abstract this even further?
internal void
load_game_assets(GameAssets *assets, PlatformAPI *platform_api, ThreadWorkQueue *gpu_work_queue, void *device)
{
#if 0
    load_font("/Users/mekalopo/Library/Fonts/LiberationMono-Regular.ttf",
                platform_api, device, &assets->font_asset, ' ', '~');
#endif
    u32 max_glyph_count = 2048;
    LoadFontInfo load_font_info = {};
    begin_load_font(&load_font_info, &assets->debug_font_asset, 
                    "/System/Library/Fonts/Supplemental/applemyungjo.ttf", platform_api, device,
                    max_glyph_count, 128.0f);
    // space works just like other glyphs, but without any texture
    add_glyph_asset(&load_font_info, gpu_work_queue, ' ');
    for(u32 codepoint = '!';
            codepoint <= '~';
            ++codepoint)
    {
        add_glyph_asset(&load_font_info, gpu_work_queue, codepoint);
    }
    add_glyph_asset(&load_font_info, gpu_work_queue, 0x8349);
    add_glyph_asset(&load_font_info, gpu_work_queue, 0x30a8);
    add_glyph_asset(&load_font_info, gpu_work_queue, 0x30f3);
    add_glyph_asset(&load_font_info, gpu_work_queue, 0x30b8);
    end_load_font(&load_font_info);
}

internal f32
get_glyph_kerning(FontAsset *font_asset, f32 scale, u32 unicode_codepoint0, u32 unicode_codepoint1)
{
    u16 glyph0ID = font_asset->codepoint_to_glyphID_table[unicode_codepoint0];
    u16 glyph1ID = font_asset->codepoint_to_glyphID_table[unicode_codepoint1];

    f32 result = scale*font_asset->kerning_advances[glyph0ID*font_asset->max_glyph_count + glyph1ID];
    return result;
}

internal f32
get_glyph_x_advance_px(FontAsset *font_asset, f32 scale, u32 unicode_codepoint)
{
    u16 glyphID = font_asset->codepoint_to_glyphID_table[unicode_codepoint];
    f32 result = scale*font_asset->glyph_assets[glyphID].x_advance_px;

    return result;
}

internal f32
get_glyph_left_bearing_px(FontAsset *font_asset, f32 scale, u32 unicode_codepoint)
{
    u16 glyphID = font_asset->codepoint_to_glyphID_table[unicode_codepoint];
    f32 result = scale*font_asset->glyph_assets[glyphID].left_bearing_px;

    return result;
}
