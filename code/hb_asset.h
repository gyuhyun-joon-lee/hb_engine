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

struct FontAssetInfo
{
    v2 pixel_min;
    v2 pixel_dim;
    f32 pixel_x_advance;
     
    // top-down, 0 to 1 range
    v2 texcoord_min;
    v2 texcoord_max;
};

struct GameAssets
{
    FontAssetInfo font_infos[256]; 
    TextureAsset font_bitmap;
};

#endif

