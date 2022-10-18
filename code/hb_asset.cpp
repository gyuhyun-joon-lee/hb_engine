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
load_font(const char *file_path, PlatformAPI *platform_api, void *device, TextureAsset *font_asset, FontAssetInfo *font_infos)
{
    PlatformReadFileResult font_data = platform_api->read_file(file_path);

    u32 font_bitmap_width = 1024;
    u32 font_bitmap_height = 1024;

    // TODO(gh) Get rid of malloc
    stbtt_bakedchar *char_infos = (stbtt_bakedchar *)malloc(sizeof(stbtt_bakedchar)*256); // only holds the ascii characters
    u8 *font_bitmap = (u8 *)malloc(sizeof(u8)*font_bitmap_width*font_bitmap_height);

    if(stbtt_BakeFontBitmap(font_data.memory, 0, 64.0f, font_bitmap, font_bitmap_width, font_bitmap_height, 0, 256, char_infos) > 0) // no guarantee this fits!
    {
        *font_asset = load_asset_texture(platform_api, device, 
                                        font_bitmap, font_bitmap_width, font_bitmap_height, 1);
    }

    for(u32 c = 0;
            c < 256;
            ++c)
    {
        stbtt_bakedchar *char_info = char_infos + c;
        FontAssetInfo *font_info = font_infos + c;

        font_info->pixel_min.x = char_info->xoff;
        font_info->pixel_min.y = 0;
        font_info->pixel_x_advance = char_info->xadvance;
        font_info->pixel_dim.x = char_info->x1 - char_info->x0;
        font_info->pixel_dim.y = char_info->y1 - char_info->y0;
        font_info->texcoord_min = V2(char_info->x0/(f32)font_bitmap_width, char_info->y0/(f32)font_bitmap_height); 
        font_info->texcoord_max = V2(char_info->x1/(f32)font_bitmap_width, char_info->y1/(f32)font_bitmap_height); 
    }

    free(char_infos);
    free(font_bitmap);
}

// TODO(gh) 'stream' assets?
// TODO(gh) This is pretty much it when we need the mtldevice for resource allocations,
// but can we abstract this even further?
internal void
load_game_assets(GameAssets *assets, PlatformAPI *platform_api, void *device)
{
    load_font("/Users/mekalopo/Library/Fonts/LiberationMono-Regular.ttf",
                platform_api, device, &assets->font_bitmap, assets->font_infos);
}
