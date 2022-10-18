/*
 * Written by Gyuhyun Lee
 */

#ifndef HB_ASSET_H
#define HB_ASSET_H

struct AssetTexture
{
    void *handle;

    i32 width;
    i32 height;
    i32 bytes_per_pixel;
};

struct GameAssets
{
    AssetTexture font_bitmap;
};

#endif

