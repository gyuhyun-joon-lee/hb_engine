/*
 * Written by Gyuhyun Lee
 */
#include "hb_asset.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

internal TextureAsset
load_texture_asset(PlatformAPI *platform_api, void *device, void *src, i32 width, i32 height, i32 bytes_per_pixel)
{
    TextureAsset result = {};

    assert(device && src);

    // Allocate the space in GPU and get the handle
    result.handle = platform_api->allocate_and_acquire_texture_handle(device, width, height, bytes_per_pixel);
    result.width = width;
    result.height = height;
    result.bytes_per_pixel = bytes_per_pixel;

    // Load the file into texture
    platform_api->write_to_entire_texture(result.handle, src, width, height, bytes_per_pixel);

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
    // TODO(gh) memset to 0?
    u32 codepoint_to_glyphID_table_size = sizeof(u32) * MAX_UNICODE_CODEPOINT;
    font_asset->codepoint_to_glyphID_table = (u32 *)malloc(sizeof(u32) * MAX_UNICODE_CODEPOINT);
    zero_memory(font_asset->codepoint_to_glyphID_table, codepoint_to_glyphID_table_size);
    font_asset->glyph_assets = (GlyphAsset *)malloc(sizeof(GlyphAsset) * max_glyph_count);
    font_asset->kerning_advances = (f32 *)malloc(sizeof(f32) * max_glyph_count * max_glyph_count);

    // glyph_infos are already relative to the pixel height that we provided,
    // but some information such as kerning, ascent, descent are not scaled properly, which means we have to
    // calculate them seperately.
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

            if(codepoint0 == 'i' && codepoint1 == 'n')
            {
                int a = 1;
            }

            f32 kerning = load_font_info->font_scale*stbtt_GetCodepointKernAdvance(&load_font_info->font_info, codepoint0 ,codepoint1);

            font_asset->kerning_advances[i*font_asset->max_glyph_count + j] = kerning;
        }
    }

    load_font_info->font_asset = 0;
}
#endif

internal void
add_glyph_asset(LoadFontInfo *load_font_info, PlatformAPI *platform_api, u32 unicode_codepoint)
{
    int glyphID_that_doesnt_exist = stbtt_FindGlyphIndex(&load_font_info->font_info, 11111);     
    assert(stbtt_FindGlyphIndex(&load_font_info->font_info, unicode_codepoint)!=glyphID_that_doesnt_exist);

    u32 glyphID = load_font_info->populated_glyph_count++;
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
        glyph_asset->texture = load_texture_asset(platform_api, load_font_info->device, bitmap, width, height, 1);
        stbtt_FreeBitmap(bitmap, 0);
    }
}

// TODO(gh) 'stream' assets?
// TODO(gh) This is pretty much it when we need the mtldevice for resource allocations,
// but can we abstract this even further?
internal void
load_game_assets(GameAssets *assets, PlatformAPI *platform_api, void *device)
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
    add_glyph_asset(&load_font_info, platform_api, ' ');
    for(u32 codepoint = '!';
            codepoint <= '~';
            ++codepoint)
    {
        add_glyph_asset(&load_font_info, platform_api, codepoint);
    }
    add_glyph_asset(&load_font_info, platform_api, 0x8349);
    add_glyph_asset(&load_font_info, platform_api, 0x30a8);
    add_glyph_asset(&load_font_info, platform_api, 0x30f3);
    add_glyph_asset(&load_font_info, platform_api, 0x30b8);
    end_load_font(&load_font_info);
}

internal f32
get_glyph_kerning(FontAsset *font_asset, f32 scale, u32 unicode_codepoint0, u32 unicode_codepoint1)
{
    u32 glyph0ID = font_asset->codepoint_to_glyphID_table[unicode_codepoint0];
    u32 glyph1ID = font_asset->codepoint_to_glyphID_table[unicode_codepoint1];

    f32 result = scale*font_asset->kerning_advances[glyph0ID*font_asset->max_glyph_count + glyph1ID];
    return result;
}

internal f32
get_glyph_x_advance_px(FontAsset *font_asset, f32 scale, u32 unicode_codepoint)
{
    u32 glyphID = font_asset->codepoint_to_glyphID_table[unicode_codepoint];
    f32 result = scale*font_asset->glyph_assets[glyphID].x_advance_px;

    return result;
}

internal f32
get_glyph_left_bearing_px(FontAsset *font_asset, f32 scale, u32 unicode_codepoint)
{
    u32 glyphID = font_asset->codepoint_to_glyphID_table[unicode_codepoint];
    f32 result = scale*font_asset->glyph_assets[glyphID].left_bearing_px;

    return result;
}
