/*
 * Written by Gyuhyun Lee
 */
#include "hb_asset.h"

internal AssetTexture
load_asset_texture(const char *file_path, PlatformAPI *platform_api, void *device, i32 width, i32 height, i32 bytes_per_pixel)
{
    AssetTexture result = {};

    // Allocate the space in GPU and get the handle
    result.handle = platform_api->allocate_and_acquire_texture_handle(device, width, height, bytes_per_pixel);
    result.width = width;
    result.height = height;
    result.bytes_per_pixel = bytes_per_pixel;

    // Load the file into texture
    PlatformReadFileResult file = platform_api->read_file(file_path);
    platform_api->write_to_entire_texture(result.handle, file.memory, width, height, bytes_per_pixel);

    return result;
}

// TODO(gh) 'stream' assets?
// TODO(gh) This is pretty much it when we need the mtldevice for resource allocations,
// but can we abstract this even further?
internal void
load_game_assets(GameAssets *assets, PlatformAPI *platform_api, void *device)
{
    assets->font_bitmap = load_asset_texture("/Users/mekalopo/Library/Fonts/LiberationMono-Regular.ttf", 
                                            platform_api, device, 
                                            1024, 1024, 1);
}
