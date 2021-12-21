#ifndef MEKA_FONT_H
#define MEKA_FONT_H

#pragma pack(push, 1)
// NOTE(joon): header of the 'head' block 
struct otf_head_block_header
{
    u16 major_version;
    u16 minor_version;
    i32 font_revision;
    u32 check_sum_adjustment;
    u32 magic_number; // should be 0x5f0f3cf5

    u16 flags;
    u16 unit_per_em; // resolution
    i64 time_created;
    i64 time_modified;
    i16 x_min;
    i16 y_min;
    i16 x_max;
    i16 y_max;
    u16 style;
    u16 min_resolution_in_pixel;
    i16 ignored; // should be 2
    i16 glyph_offset_type; 
    i16 glyph_data_format; // should be 0 
};

struct otf_table_record
{
    u32	table_tag; //	Table identifier.
    u32	check_sum; //	Checksum for this table.
    u32	offset; //	Offset from beginning of font file.
    u32	length; //	Length of this table.
};

// NOTE(joon): otf file starts with this
// NOTE(joon): Every otf file is in big endian...
struct otf_header
{
    u32 version; // should be 0x00010000 or 0x4F54544F ('OTTO')
    u16 table_count;
    u16 search_range;
    u16 entry_selector; // 
    u16 range_shift; // numTables times 16, minus searchRange ((numTables * 16) - searchRange).
};

struct otf_EBLC_block_header
{
    u16 major_version; // should be 2
    u16 minor_version; // should be 0
    u32 bitmap_size_record_count;
};

struct subline_metric
{
    i8 ascender;
    i8 descender;
    u8 widthMax;
    i8 caretSlopeNumerator;
    i8 caretSlopeDenominator;
    i8 caretOffset;
    i8 minOriginSB;
    i8 minAdvanceSB;
    i8 maxBeforeBL;
    i8 minAfterBL;

    i8 pad1;
    i8 pad2;
};


// NOTE(joon): Stored in EBLC block, stores offset to EBDT Table & the format for each glyph
struct otf_bitmap_size
{
    u32	offset_to_index_subtable; // Offset to IndexSubtableArray, from beginning of EBLC.
    u32	index_subtable_size;
    u32	index_subtable_count; // TODO(joon): Why is there a size _and_ count for index subtable?
    u32	ignored0; // must be 0.

    subline_metric horizontal_metric;
    subline_metric vertical_metric;

    u16	startGlyphIndex;	// Lowest glyph index that this uses this bitmap size.
    u16	endGlyphIndex;	// Highest glyph index that this uses this bitmap size

    u8	horizontal_pixel_count_per_resolution; // this 'unit' is represented in the 'head' by resolution
    u8	vertical_pixel_count_per_resolution;

    u8	bitDepth; // 	The Microsoft rasterizer v.1.7 or greater supports the following bitDepth values, as described below: 1, 2, 4, and 8.
    i8	flags;	//Vertical or horizontal (see Bitmap Flags, below).
};


struct otf_cmap_block_header
{
    u16 version;
    u16 entry_count;
};

struct otf_encoding
{
    u16 type;
    u16 subtype;
    u32 subtable_offset;
};

struct otf_maxp_block_header
{
    // NOTE(joon): Just includes maxp header from version 0.5
    u32 version;
    u16 glyph_count;
};

struct otf_glyf_block_header
{
    i16 number_of_contours;

    i16 x_min;
    i16 y_min;

    i16 x_max;
    i16 y_max;
};

struct font_info
{
    u32 glyph_count;
    otf_bitmap_size *bitmap_sizes;

    u32 resolution;

    u8 glyph_offset_type; // 0 means the element of loca array is 16 bit, and 1 means it's 32 bit
    void *glyph_offsets; // Access to glyf tabletype is undefined because it can be differ based on loc_offset_type

    u8 *glyph_data;
};
#pragma pack(pop)

#endif
