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
    f32 x_offset_px; // Where should we draw this glyphs based on the current position x
    f32 x_advance_px; // How much we should advance to draw the next charcter
    // Most upper case will be 0, and some characters like 'p' will be -12 or something,
    // which means the character extends below the baseline
    f32 y_offset_from_baseline_px; 

    v2 dim_px;
     
    // top-down, 0 to 1 range
    v2 texcoord_min01;
    v2 texcoord_max01;
};

struct GameAssets
{
    GlyphAssetInfo glyph_infos[256]; 
    TextureAsset font_bitmap;
};

#endif

