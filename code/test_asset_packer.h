#ifndef TEST_ASSET_PACKER_H
#define TEST_ASSET_PACKER_H

struct PlatformReadFileResult
{
    u8 *memory;
    u64 size; // TOOD/gh : make this to be at least 64bit
};

struct LoadFontInfo
{
    u32 glyph_count;

    // table used to get glyphID
    u16 *codepoint_to_glyphID_table; 
    
    f32 ascent_from_baseline;
    f32 descent_from_baseline;
    f32 line_gap;
    
    stbtt_fontinfo font_info;
    f32 font_scale;
    f32 desired_font_height_px;
};

#pragma pack(push, 1)
struct PackedGlyphAsset
{

    f32 left_bearing_px;
    f32 x_advance_px;

    f32 x_offset_px;
    f32 y_offset_from_baseline_px; 

    v2 dim_px;

    u32 unicode_codepoint;

    i32 bitmap_width;
    i32 bitmap_height;

    // NOTE(gh) Next memory is the bitmap
    // unsigned char *bitmap;
};
#pragma pack(pop)

#endif
