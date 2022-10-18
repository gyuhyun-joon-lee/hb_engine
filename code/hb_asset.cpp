/*
 * Written by Gyuhyun Lee
 */
#include "hb_asset.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

internal AssetTexture
load_asset_texture(PlatformAPI *platform_api, void *device, void *src, i32 width, i32 height, i32 bytes_per_pixel)
{
    AssetTexture result = {};

    // Allocate the space in GPU and get the handle
    result.handle = platform_api->allocate_and_acquire_texture_handle(device, width, height, bytes_per_pixel);
    result.width = width;
    result.height = height;
    result.bytes_per_pixel = bytes_per_pixel;

    // Load the file into texture
    platform_api->write_to_entire_texture(result.handle, src, width, height, bytes_per_pixel);

    return result;
}

internal AssetTexture
load_font_bitmap_texture(const char *file_path, PlatformAPI *platform_api, void *device)
{
    AssetTexture result = {};

    PlatformReadFileResult font_data = platform_api->read_file(file_path);

    u32 font_bitmap_width = 1024;
    u32 font_bitmap_height = 1024;
    // TODO(gh) Get rid of malloc
    stbtt_bakedchar *char_infos = (stbtt_bakedchar *)malloc(sizeof(stbtt_bakedchar)*256); // only holds the ascii characters
    u8 *font_bitmap = (u8 *)malloc(sizeof(u8)*font_bitmap_width*font_bitmap_height);

    if(stbtt_BakeFontBitmap(font_data.memory, 0, 64.0f, font_bitmap, font_bitmap_width, font_bitmap_height, 0, 256, char_infos) > 0) // no guarantee this fits!
    {
        result = load_asset_texture(platform_api, device, 
                                    font_bitmap, font_bitmap_width, font_bitmap_height, 1);
    }

    return result;
}

// TODO(gh) 'stream' assets?
// TODO(gh) This is pretty much it when we need the mtldevice for resource allocations,
// but can we abstract this even further?
internal void
load_game_assets(GameAssets *assets, PlatformAPI *platform_api, void *device)
{
    assets->font_bitmap = load_font_bitmap_texture("/Users/mekalopo/Library/Fonts/LiberationMono-Regular.ttf",
                                                    platform_api, device);
}
