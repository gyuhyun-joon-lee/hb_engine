#include <sys/stat.h>
#include <Cocoa/Cocoa.h> 

#undef internal
#undef assert

#include "hb_types.h"
#include "hb_math.h"
#include "hb_intrinsic.h"
#include "stb_truetype.h"
#include "test_asset_packer.h"

#define MAX_UNICODE_CODEPOINT 0x10FFFF

internal u64
macos_get_file_size(const char *filename) 
{
    u64 result = 0;

    int File = open(filename, O_RDONLY);
    struct stat FileStat;
    fstat(File , &FileStat); 
    result = FileStat.st_size;
    close(File);

    return result;
}

internal PlatformReadFileResult
macos_read_file(const char *filename)
{
    PlatformReadFileResult result = {};

    int File = open(filename, O_RDONLY);
    int Error = errno;
    if(File >= 0) // NOTE : If the open() succeded, the return value is non-negative value.
    {
        struct stat FileStat;
        fstat(File , &FileStat); 
        off_t fileSize = FileStat.st_size;

        if(fileSize > 0)
        {
            // TODO/gh : no more os level allocations!
            result.size = fileSize;
            result.memory = (u8 *)malloc(result.size);
            if(read(File, result.memory, result.size) == -1)
            {
                free(result.memory);
                result.size = 0;
            }
        }

        close(File);
    }

    return result;
}

internal void
debug_macos_write_entire_file(const char *file_name, void *memory_to_write, u32 size)
{
    int file = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);

    if(file >= 0) 
    {
        if(write(file, memory_to_write, size) == -1)
        {
            // TODO(gh) : log
        }

        close(file);
    }
    else
    {
        // TODO(gh) :log
        printf("Failed to create file\n");
    }
}

internal void
debug_macos_free_file_memory(void *memory)
{
    free(memory);
}

internal void
begin_load_font(LoadFontInfo *load_font_info, const char *file_path, 
                f32 desired_font_height_px)
{
    PlatformReadFileResult font_data = macos_read_file(file_path);
    load_font_info->desired_font_height_px = desired_font_height_px;

    stbtt_InitFont(&load_font_info->font_info, font_data.memory, 0);
    load_font_info->font_scale = stbtt_ScaleForPixelHeight(&load_font_info->font_info, desired_font_height_px);

    int ascent;
    int descent;
    int line_gap;
    stbtt_GetFontVMetrics(&load_font_info->font_info, &ascent, &descent, &line_gap);

    load_font_info->ascent_from_baseline = load_font_info->font_scale * ascent;
    load_font_info->descent_from_baseline = -1.0f*load_font_info->font_scale * descent; // stb library gives us negative value, but we want positive value for this
    load_font_info->line_gap = load_font_info->font_scale*line_gap;

    // The size of this table has to be  MAX_UNICODE_CODEPOINT, 
    // as we need some sort of entry point and all we have is unicode codepoint
    u32 codepoint_to_glyphID_table_size = sizeof(u32) * MAX_UNICODE_CODEPOINT;
    load_font_info->codepoint_to_glyphID_table = (u16 *)malloc(sizeof(u16) * MAX_UNICODE_CODEPOINT);
    zero_memory(load_font_info->codepoint_to_glyphID_table, codepoint_to_glyphID_table_size);

    // TODO(gh) Should do this when we get the exact glyph count
}


internal void
add_glyph_asset(LoadFontInfo *load_font_info, u32 unicode_codepoint)
{
    load_font_info->glyph_count++;

    // fwrite(tag);
    // fwrite(value);

    int x_advance_px;
    int left_bearing_px;
    stbtt_GetCodepointHMetrics(&load_font_info->font_info, unicode_codepoint, &x_advance_px, &left_bearing_px);

    PackedGlyphAsset packed_asset;
    packed_asset.unicode_codepoint = unicode_codepoint;
    packed_asset.left_bearing_px = load_font_info->font_scale*left_bearing_px;
    packed_asset.x_advance_px = load_font_info->font_scale*x_advance_px;
     
    int width;
    int height;
    int x_off;
    int y_off;
    unsigned char *bitmap = stbtt_GetCodepointBitmap(&load_font_info->font_info, 
                                                    load_font_info->font_scale, load_font_info->font_scale,
                                                    unicode_codepoint,
                                                    &width, &height, &x_off, &y_off);
    // scale is already baked in
    packed_asset.dim_px = V2(width, height);
    packed_asset.x_offset_px = x_off;
    packed_asset.y_offset_from_baseline_px = -(height + y_off); 

    packed_asset.bitmap_width = width;
    packed_asset.bitmap_height = height;
    u32 bitmap_size = sizeof(unsigned char) *
                      packed_asset.bitmap_width *
                      packed_asset.bitmap_height;

    // fwrite(packed_asset);
    // fwrite(bitmap);
    
    stbtt_FreeBitmap(bitmap, 0);
}

internal void
end_load_font(LoadFontInfo *load_font_info)
{
    // TODO(gh) Need to write this out to the packed asset file
    f32 *kerning_advances = (f32 *)malloc(sizeof(f32) * 
                                          load_font_info->glyph_count *
                                          load_font_info->glyph_count);
}

int main(void)
{
    return 0;
}
