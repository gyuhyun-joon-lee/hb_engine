/*
 * Written by Gyuhyun Lee
 */
#include "hb_asset.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

internal TextureAsset
load_asset_texture(PlatformAPI *platform_api, void *device, void *src, i32 width, i32 height, i32 bytes_per_pixel)
{
    TextureAsset result = {};

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
load_font(const char *file_path, PlatformAPI *platform_api, void *device, 
        FontAsset *font_asset, u32 start_glyph, u32 end_glyph)
{
    PlatformReadFileResult font_data = platform_api->read_file(file_path);

    font_asset->glyph_count = end_glyph - start_glyph + 1;
    font_asset->start_glyph = start_glyph;
    font_asset->end_glyph = end_glyph;
    // TODO(gh) We should get rid of this malloc, for sure.
    font_asset->glyph_infos = (GlyphAssetInfo *)malloc(sizeof(GlyphAssetInfo) * font_asset->glyph_count);
    font_asset->kerning_advances = (f32 *)malloc(sizeof(f32) * font_asset->glyph_count * font_asset->glyph_count);

    // TODO(gh) Also make these configurable?
    u32 font_bitmap_width = 2048;
    u32 font_bitmap_height = 2048;
    f32 desired_font_height_px = 128.0f;

    // TODO(gh) Get rid of malloc, or do we even care because this thing should be in 
    // asset builder, not in game code
    stbtt_bakedchar *char_infos = (stbtt_bakedchar *)malloc(sizeof(stbtt_bakedchar)*font_asset->glyph_count); // only holds the ascii characters
    u8 *font_bitmap = (u8 *)malloc(sizeof(u8)*font_bitmap_width*font_bitmap_height);

    // Load font bitmap into GPU
    if(stbtt_BakeFontBitmap(font_data.memory, 0, (int)desired_font_height_px, font_bitmap, font_bitmap_width, font_bitmap_height, start_glyph, font_asset->glyph_count, char_infos) > 0) // no guarantee this fits!
    {
        font_asset->font_bitmap = load_asset_texture(platform_api, device, 
                                        font_bitmap, font_bitmap_width, font_bitmap_height, 1);
        free(font_bitmap);
    }
    else
    {
        // TODO(gh) So the bitmap cannot contain all the glyphs that we requested... what should we do here?
        assert(0);
    }

    for(u32 i = 0;
            i < font_asset->glyph_count;
            ++i)
    {
        stbtt_bakedchar *char_info = char_infos + i;
        GlyphAssetInfo *glyph_info = font_asset->glyph_infos + i;

        int height = char_info->y1 - char_info->y0;

        glyph_info->x_offset_px = char_info->xoff;
        glyph_info->x_advance_px = char_info->xadvance;
        glyph_info->y_offset_from_baseline_px = -(height + char_info->yoff); // STB uses top-down bitmap, and yoff is also top-down in px based on the glyph's height

        glyph_info->dim_px = V2(char_info->x1 - char_info->x0, char_info->y1 - char_info->y0);

        // Top-down, ranging from 0 to 1
        glyph_info->texcoord_min01 = V2(char_info->x0/(f32)font_bitmap_width, char_info->y0/(f32)font_bitmap_height); 
        glyph_info->texcoord_max01 = V2(char_info->x1/(f32)font_bitmap_width, char_info->y1/(f32)font_bitmap_height); 
    }
    free(char_infos);

#if 1
    // glyph_infos are already relative to the pixel height that we provided,
    // but some information such as kerning is not, which means we have to
    // calculate them seperately.
    stbtt_fontinfo font_info = {};
    stbtt_InitFont(&font_info, font_data.memory, 0);
    f32 font_scale = stbtt_ScaleForPixelHeight(&font_info, desired_font_height_px);

    for(u32 i = 0;
            i < font_asset->glyph_count;
            ++i)
    {
        for(u32 j = 0;
                j < font_asset->glyph_count;
                ++j)
        {
            u32 glyph0 = i + font_asset->start_glyph;
            u32 glyph1 = j + font_asset->start_glyph;
            int kern_advance = stbtt_GetCodepointKernAdvance(&font_info, (int)glyph0, (int)glyph1);

            if(kern_advance !=0)
            {
                int a =1;
            }

            if(glyph0 == 'T' && glyph1 == 'o')
            {
                int a = 1;
            }

            // indexing is first_glyphID * glyp_count + second_glyphID
            font_asset->kerning_advances[i*font_asset->glyph_count + j] = font_scale*kern_advance;
        }
    }

    int ascent;
    int descent;
    int line_gap;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
    // The math is directly from stb_truetype.h
    font_asset->newline_advance_px = font_scale*(ascent-descent+line_gap);
#endif
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
    load_font("/System/Library/Fonts/Supplemental/arial.ttf",
                platform_api, device, &assets->font_asset, ' ', '~');
}

internal f32
get_font_kerning(FontAsset *font_asset, u32 glyph0, u32 glyph1)
{
    u32 glyph0ID = glyph0 - font_asset->start_glyph;
    u32 glyph1ID = glyph1 - font_asset->start_glyph;

    f32 result = font_asset->kerning_advances[glyph0ID*font_asset->glyph_count + glyph1ID];
    return result;
}
