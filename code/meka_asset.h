#ifndef MEKA_ASSET_H
#define MEKA_ASSET_H

enum AssetType
{
    AssetType_Null,

    AssetType_Mesh,
    AssetType_Voxel,

    AssetType_Count,
};

enum AssetState
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
    AssetState_Locked, // TODO(joon) what asset should be marked as locked?
};

struct AssetSlot
{
    AssetState state;

    u32 first_tag_index;
    u32 tag_count;
    union
    {
        // NOTE(joon) pointers to the memory that acutally has the asset(and asset infos)
        // loaded_bitmap
        // loaded_wave
    };
};

// NOTE(joon) based on the type id of the asset
// NOTE(joon) used to search the slots with certain type
struct AssetSlotInfo
{
    u32 first_slot_index;
    u32 slot_count_for_this_type;
};

struct AssetTag
{
    u32 ID;
    f32 value;
};

struct GameAssets
{
    AssetTag *tags;
    u32 tag_count;

    AssetSlotInfo slot_infos[AssetType_Count];
    u32 slot_info_count;

    AssetSlot *slots;
    u32 slot_count;
};

#endif

