/*
 * Written by Gyuhyun Lee
 */

#ifndef HB_ASSET_H
#define HB_ASSET_H

struct TextureAsset
{
    void *handle;

    i32 width;
    i32 height;
    i32 bytes_per_pixel;
};

struct GlyphAssetInfo
{
    // These are all scaled based on the requested font height
    f32 x_offset_px; // Where should we draw this glyphs based on the current position x
    f32 x_advance_px; // How much we should advance to draw the next character(does not take account of kerning)
    f32 y_offset_from_baseline_px; 

    v2 dim_px;
     
    // top-down, 0 to 1 range
    v2 texcoord_min01;
    v2 texcoord_max01;
};

struct FontAsset
{
    u32 start_glyph;
    u32 end_glyph;
    u32 glyph_count;
    f32 *kerning_advances; // can hold glyph_count*glyph_count amount of data
    GlyphAssetInfo *glyph_infos; // can hold glyph_count amount of data 

    f32 newline_advance_px; // how much we should advance vertically, when we move onto newline

    TextureAsset font_bitmap;
};

struct GameAssets
{
    FontAsset font_asset;
};

#endif

