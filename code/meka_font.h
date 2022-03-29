#ifndef MEKA_FONT_H
#define MEKA_FONT_H
/*
   NOTE(joon): otf file blocks

   -required
   'cmap'	Character to glyph mapping
   'head'	Font header
   'hhea'	Horizontal header
   'hmtx'	Horizontal metrics
   'maxp'	Maximum profile
   'name'	Naming table
   OS/2	OS/2 and Windows specific metrics
   'post'	PostScript information

   -outline 
   'cvt '	Control Value Table (optional table)
   'fpgm'	Font program (optional table)
   'glyf'	Glyph data
   'loca'	Index to location
   'prep'	CVT Program (optional table)
   'gasp'	Grid-fitting/Scan-conversion (optional table)

   -bitmap // may not be included in the font file
   EBDT	Embedded bitmap data
   EBLC	Embedded bitmap location data
   EBSC	Embedded bitmap scaling data
   CBDT	Color bitmap data
   CBLC	Color bitmap location data
   'sbix'	Standard bitmap graphics

   -vector
*/

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

// TODO(joon) not used?
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

struct otf_cmap_block_header
{
    u16 version;
    u16 entry_count;
};

struct otf_encoding
{
    u16 type;
    u16 subtype;
    u32 subtable_offset; // based on the beginning of the 'cmap' block
};

// NOTE(joon) Can be different based on the encoding type(unicode1.0 vs unicode 2.0, windows, makintosh...etc)
// IF the platform ID was 0(unicode) & encoding ID was 3, format can be 4 or 6(should check the first element of encoding subtable header)
struct otf_encoding_subtable_format_4_header
{
    // NOTE(joon) For the types of endcoding subtable format,
    // https://docs.microsoft.com/en-us/typography/opentype/spec/cmap
    u16 format; // should be 4
    u16 length; // This is the length in bytes of the subtable.
    u16 language;	// For requirements on use of the language field, see “Use of the language field in 'cmap' subtables” in this document.
    u16 twice_of_segment_count; // 2 × segCount.

    // NOTE(joon) glyph ID search accelerator using the binary tree
    u16 search_range; // Maximum power of 2 less than or equal to segCount, times 2 ((2**floor(log2(segCount))) * 2, where “**” is an exponentiation operator)
    u16 entry_selector; // Log2 of the maximum power of 2 less than or equal to numTables (log2(searchRange/2), which is equal to floor(log2(numTables)))
    u16 range_shift; //segCount times 2, minus searchRange ((segCount * 2) - searchRange)

    /*
       NOTE(joon) Rest of them looks like this, and we cannot store inside the format header as most of them are variable sized
       uint16 endCode[segCount]; // End characterCode for each segment, last=0xFFFF.
       uint16 reserved;	// Set to 0.
       uint16 startCode[segCount]; // Start character code for each segment.
       int16 idDelta[segCount]; // Delta for all character codes in segment.
       uint16 idRangeOffsets[segCount]; //Offsets into glyphIdArray or 0
       uint16 glyphIdArray[ ]; //Glyph index array (arbitrary length)
    */
};


struct otf_maxp_block_header
{
    // NOTE(joon) Just includes maxp header from version 0.5
    u32 version;
    u16 glyph_count;
};

// _Each_ data block inside the 'glyf' block starts with this header
struct otf_glyf_desc
{
    i16 number_of_contours;

    i16 x_min;
    i16 y_min;

    i16 x_max;
    i16 y_max;
};


struct otf_index_subtable_header
{
    u16	first_glyph_index; //First glyph ID of this range.
    u16	last_glyph_index; //Last glyph ID of this range (includes this ID).
    u32	additional_offset_to_index_subtable; //Add to indexSubTableArrayOffset to get offset from beginning of EBLC.
};

// NOTE(joon): Stored in EBLC block, stores offset to EBDT Table & the format for each glyph
struct otf_bitmap_size
{
    u32	offset_to_index_subtable; // Offset to IndexSubtableArray, from beginning of EBLC.

    // NOTE(joon): glyphs that have same property(i.e size of the bounding box) can share same index subtable -> which is why there are multiple index subtables
    u32	index_subtable_size; // per each index subtable
    u32	index_subtable_count; 

    u32	ignored0; // must be 0.

    subline_metric horizontal_metric;
    subline_metric vertical_metric;

    // NOTE(joon): Does not contain all the glyphs in this range
    u16	min_glyph_index;
    u16	max_glyph_index;

    u8	horizontal_pixel_count_per_resolution; // this 'unit' is represented in the 'head' by resolution
    u8	vertical_pixel_count_per_resolution;

    u8	bit_depth; // 	The Microsoft rasterizer v.1.7 or greater supports the following bitDepth values, as described below: 1, 2, 4, and 8.
    i8	flags;	//Vertical or horizontal (see Bitmap Flags, below).
};
#pragma pack(pop)

struct otf_glyph_id_finder
{
    u16 segment_count;

    // NOTE(joon) count = segment_count 
    u16 *start_character_codes;
    u16 *end_character_codes;
    i16 *id_deltas; // Delta for all character codes in segment.
    u16 *id_range_offsets; //Offsets into glyphIdArray or 0

    // count = arbitrary, only used if id_range_offset value is non-zero
    u16 *glyph_id_array; 
};

struct font_info
{
    u32 glyph_count;
    u32 resolution;

    //////////// bitmap related data ////////////
    // NOTE(joon): The reason why we are storing this is because 'additional offset' in otf_index_subtable_header & offset in otf_bitmap_size
    // are both relative to the start of eblc block
    u8 *start_of_eblc_block;
    u32 bitmap_size_count;
    otf_bitmap_size *bitmap_sizes; // NOTE(joon): This contains min-max glyph indices that can be represented with certain bitmap size info struct

    //////////// outline related data ////////////
    u8 glyph_offset_type; // 0 means the element of loca array is 16 bit, and 1 means it's 32 bit
    void *glyph_offsets; // Access to glyf tabletype is undefined because it can be differ based on loc_offset_type
    u8 *glyph_data; // Also a start of the 'glyf' header

    otf_glyph_id_finder glyph_id_finder;
};


#endif
