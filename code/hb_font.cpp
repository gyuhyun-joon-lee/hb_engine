#include "hb_font.h"

#if 0
    /*
       NOTE(joon): How to get glyph bitmap information
       1. Get the glyph ID - For our case, there's a high chance of ASCII code being same as glyph ID 
       2. Go to bitmap size array which is located in 'EBLC' block
       3. Loop through each block, and check if the glyph ID is located between min & max glyph index.
       4. If so, loop through index subtable and check the index subtable header if the subtable includes glyph ID, as the range above does not contain all the glyph IDs between
       5. If we succefully found the index subtable, use the additional offset to get the actual information about the glyph

       6. Somehow use horizontal & vertical metric & index subtable to get the glyph bitmap & info???
    */

 

    for(u32 bitmap_size_index = 0;
            bitmap_size_index < font_info.bitmap_size_count;
            ++bitmap_size_index)
    {
        // TODO(joon): Converting from big to little endian every time I have to access the font table 
        // seems a bit wasteful... Allocate memory to store these converted values? 
        otf_bitmap_size *bitmap_size = (otf_bitmap_size *)font_info.bitmap_sizes + bitmap_size_index;
        u32	offset_to_index_subtable = big_to_little_endian(bitmap_size->offset_to_index_subtable); // Offset to IndexSubtableArray, from beginning of EBLC.
        u32	index_subtable_size = big_to_little_endian(bitmap_size->index_subtable_size);
        u32	index_subtable_count = big_to_little_endian(bitmap_size->index_subtable_count); // TODO(joon): Why is there a size _and_ count for index subtable?
        u32	ignored0 = bitmap_size->ignored0; // must be 0.

        subline_metric horizontal_metric = bitmap_size->horizontal_metric;
        subline_metric vertical_metric = bitmap_size->vertical_metric;

        //u16	startGlyphIndex = big_to_little_endian(bitmap_size->startGlyphIndex);	// Lowest glyph index that this uses this bitmap size.
        //u16	endGlyphIndex = big_to_little_endian(bitmap_size->endGlyphIndex);	// Highest glyph index that this uses this bitmap size

        u8 horizontal_pixel_count_per_resolution = bitmap_size->horizontal_pixel_count_per_resolution; // this 'unit' is represented in the 'head' by resolution
        u8 vertical_pixel_count_per_resolution = bitmap_size->vertical_pixel_count_per_resolution;

        u8 bit_depth = bitmap_size->bit_depth;
        i8 flags = bitmap_size->flags;

        otf_index_subtable_header *index_subtable_array_start = (otf_index_subtable_header *)(font_info.start_of_eblc_block + offset_to_index_subtable);
        u16 first_glyph_index = big_to_little_endian(index_subtable_array_start->first_glyph_index);
        u16 last_glyph_index = big_to_little_endian(index_subtable_array_start->last_glyph_index);
        u32 additional_offset_to_index_subtable = big_to_little_endian(index_subtable_array_start->additional_offset_to_index_subtable);

        assert(first_glyph_index == 371);

        int a = 1;
    } 

    font_info font_info = load_otf(font);
    /*
       NOTE(joon): How to get glyph outline information
       1. Get the glyph ID using cmap header & otf_glyph_id_finder
       2. Get the glyph data offset from the 'loca' block, which has the offsets from the _start_ of the 'glyf' block
       3. 
    */
    u16 b_glyph_id = otf_get_glyph_id(&font_info.glyph_id_finder, 'b');
    otf_get_glyph_data(&font_info, b_glyph_id);
#endif

internal font_info
load_otf(platform_read_file_result loaded_font)
{
    u8 *c = loaded_font.memory;
    otf_header *header = (otf_header *)c;
    u32 version = big_to_little_endian(header->version);
    u32 table_count = big_to_little_endian(header->table_count);
    u32 search_range = big_to_little_endian(header->search_range);
    u32 entry_selector = big_to_little_endian(header->entry_selector);
    u32 range_shift = big_to_little_endian(header->range_shift);

    c += sizeof(otf_header);

    font_info font_info = {};

    for(u32 table_record_index = 0;
            table_record_index < table_count;
            ++table_record_index)
    {
        otf_table_record *table_record = (otf_table_record *)c + table_record_index;
        u32	table_tag = table_record->table_tag; // NOTE(joon): Not converted to little endian for direct string comparison
        u32	check_sum = big_to_little_endian(table_record->check_sum);
        u32	offset = big_to_little_endian(table_record->offset);
        u32	length = big_to_little_endian(table_record->length);

        if(table_tag == convert_cc("head"))
        {
            otf_head_block_header *header = (otf_head_block_header *)(loaded_font.memory + offset);
            i16 major_version = big_to_little_endian(header->major_version);
            i16 minor_version = big_to_little_endian(header->minor_version);
            i16 font_revision = big_to_little_endian(header->font_revision);

            u32 check_sum_adjustment = big_to_little_endian(header->check_sum_adjustment);
            u32 magic_number = header->magic_number; // should be 0x5f0f3cf5
            u16 flags = big_to_little_endian(header->check_sum_adjustment);
            font_info.resolution = (u32)big_to_little_endian(header->unit_per_em);
            //i64 time_created = big_to_little_endian(header->time_created);
            //i64 time_modified = big_to_little_endian(header->time_modified);
            i16 x_min = big_to_little_endian(header->x_min);
            i16 y_min = big_to_little_endian(header->y_min);
            i16 x_max = big_to_little_endian(header->x_max);
            i16 y_max = big_to_little_endian(header->y_max);
            u16 style = big_to_little_endian(header->style);
            u16 min_resolution_in_pixel = big_to_little_endian(header->min_resolution_in_pixel);
            i16 glyph_data_format = big_to_little_endian(header->glyph_data_format);
            font_info.glyph_offset_type = big_to_little_endian(header->glyph_offset_type); // 0 for u16, 1 for u32 for the loca block

            int a = 1;
        }
        else if(table_tag == convert_cc("cmap"))
        {
            otf_cmap_block_header *header = (otf_cmap_block_header *)(loaded_font.memory + offset);
            u16 version = big_to_little_endian(header->version);
            u16 entry_count = big_to_little_endian(header->entry_count);

            otf_encoding *encodings = (otf_encoding *)((u8 *)header + sizeof(*header));

            u16 encoding_type = big_to_little_endian(encodings->type);
            u16 encoding_subtype = big_to_little_endian(encodings->subtype);
            u32 subtable_offset = big_to_little_endian(encodings->subtable_offset);

            // TODO(joon) only supports unicode 2.0 for now
            assert(encoding_type == 0 && encoding_subtype == 3);

            otf_encoding_subtable_format_4_header *encoding_header = (otf_encoding_subtable_format_4_header *)((u8 *)header + subtable_offset);

            u16 encoding_subtable_format = big_to_little_endian(encoding_header->format);
            assert(encoding_subtable_format == 4);

            u16 twice_of_segment_count = big_to_little_endian(encoding_header->twice_of_segment_count);
            assert(twice_of_segment_count%2 == 0);
            u16 segment_count = twice_of_segment_count/2;

            font_info.glyph_id_finder.segment_count = segment_count;
            font_info.glyph_id_finder.end_character_codes = (u16 *)((u8 *)encoding_header + sizeof(*encoding_header));
            font_info.glyph_id_finder.start_character_codes = (u16 *)((u8 *)font_info.glyph_id_finder.end_character_codes + sizeof(u16)*(segment_count + 1)); // +1 is because of the reserved value between endcode and startcode
            font_info.glyph_id_finder.id_deltas = (i16 *)((u8 *)font_info.glyph_id_finder.start_character_codes + sizeof(u16)*(segment_count));
            font_info.glyph_id_finder.id_range_offsets = (u16 *)((u8 *)font_info.glyph_id_finder.id_deltas + sizeof(i16)*(segment_count));
            font_info.glyph_id_finder.glyph_id_array = (u16 *)((u8 *)font_info.glyph_id_finder.id_range_offsets + sizeof(u16)*(segment_count));
        }
        else if(table_tag == convert_cc("EBLC"))
        {
            otf_EBLC_block_header *header = (otf_EBLC_block_header *)(loaded_font.memory + offset);

            u16 major_version = big_to_little_endian(header->major_version);
            u16 minor_version = big_to_little_endian(header->minor_version);
            font_info.bitmap_size_count = big_to_little_endian(header->bitmap_size_record_count);

            font_info.start_of_eblc_block = loaded_font.memory + offset;
            font_info.bitmap_sizes = (otf_bitmap_size *)((u8 *)header + sizeof(otf_EBLC_block_header));
        }
        else if(table_tag == convert_cc("EBDT"))
        {
            // embedded Bitmap Data Table
        }
        else if(table_tag == convert_cc("maxp"))
        {
            otf_maxp_block_header *header = (otf_maxp_block_header *)(loaded_font.memory + offset);

            font_info.glyph_count = big_to_little_endian(header->glyph_count);
        }
        else if(table_tag == convert_cc("loca"))
        {
            font_info.glyph_offsets = (void *)(loaded_font.memory + offset);
        }
        else if(table_tag == convert_cc("glyf"))
        {
            font_info.glyph_data = (loaded_font.memory + offset);
        }
    }
    
    return font_info;
}

// TODO(joon) only works for format 4 for now
internal u16
otf_get_glyph_id(otf_glyph_id_finder *finder, u16 c)
{
    u16 result = 0;

    // TODO(joon) only works for ASCII codes for now
    assert(c < 0xff);

    u32 glyph_segment_index = 0;
    for(u32 i = 0;
            i < finder->segment_count;
            ++i)
    {
        if(c >= big_to_little_endian(finder->start_character_codes[i]) && 
            c <= big_to_little_endian(finder->end_character_codes[i]))
        {
            glyph_segment_index = i;
            break;
        }
    }

    if(big_to_little_endian(finder->id_range_offsets[glyph_segment_index]) == 0)
    {
        result = c + big_to_little_endian(finder->id_deltas[glyph_segment_index]);
    }
    else
    {
        result = *(&finder->id_range_offsets[glyph_segment_index] + 
                        big_to_little_endian(finder->id_range_offsets[glyph_segment_index]/2) + 
                        (c - big_to_little_endian(finder->start_character_codes[glyph_segment_index])));
    }

    return result;
}


internal void
otf_get_glyph_data(font_info *font_info, u16 glyph_id)
{
    u32 offset_to_glyph_data = 0;

    if(font_info->glyph_offset_type == 0)
    {
        offset_to_glyph_data = big_to_little_endian(*((u16 *)font_info->glyph_offsets + glyph_id));
    }
    if(font_info->glyph_offset_type == 1)
    {
        offset_to_glyph_data = big_to_little_endian(*((u32 *)font_info->glyph_offsets + glyph_id));
    }

    otf_glyf_desc *glyph_desc = (otf_glyf_desc *)(font_info->glyph_data + offset_to_glyph_data);
    // NOTE(joon): positive - simple glyph, negative - composite glyph
    // https://docs.microsoft.com/en-us/typography/opentype/spec/glyf
    i16 number_of_contours = big_to_little_endian(glyph_desc->number_of_contours); 

    i16 x_min = big_to_little_endian(glyph_desc->x_min);
    i16 y_min = big_to_little_endian(glyph_desc->y_min);

    i16 x_max = big_to_little_endian(glyph_desc->x_max);
    i16 y_max = big_to_little_endian(glyph_desc->x_max);

    if(number_of_contours >= 0)
    {
        /*
            // simple glyph description
            u16	end_points_of_contours[number_of_contours]; 
            u16	instruction_count; // If zero, no instructions are present for this glyph, and this field is followed directly by the flags field.
            u8	instructions[instruction_count]; // count == instruction_count
            u8	flags[???]; // count should be gotten by actually parsing the flags

            u8 or int16	dx_from_prev[last element of end_points_of_contours] // As the points can repeat, this does not correspond to the actual total points
            u8 or int16	dy_from_prev[last element of end_points_of_contours]  
        */

        assert(number_of_contours > 0);

        u16 *end_points_of_contours = (u16 *)((u8 *)glyph_desc + sizeof(*glyph_desc));

        u16 instruction_count = big_to_little_endian(*(u16 *)((u8 *)end_points_of_contours + sizeof(u16)*number_of_contours));
        u8 *instructions = (u8 *)end_points_of_contours + sizeof(u16)*number_of_contours + sizeof(u16);
        u8 *flags = instructions + instruction_count;

        u16 n = big_to_little_endian(end_points_of_contours[number_of_contours - 1]) + 1;

        // As we do not know how many flags there are(which means we also only know where the dxs & dys are
        // inside the simple glphy description), we have to first parse the flags
        // while  the flag_count < end_points_of_contours[number_of_contours - 1] + 1
        u32 flag_count = 0;
        u8 *f = flags;
        u32 x_coord_bytes = 0;
        u32 y_coord_bytes = 0;
        while(flag_count < n)
        {
            u8 flag = *f;
            if(flag & 0x8)
            {
                // if repeat, the next byte is the number of how many times this flag should repeat
                flag_count += *(++f);
                f++;
            }
            else
            {
                flag_count++;
                f++;
            }

            if(flag & 0x2)
            {
                // x is one byte long, and the sign depends on the 0x10 bit
                x_coord_bytes++;
            }
            else
            {
                if(flag & 0x10)
                {
                    // same as prev
                }
                else
                {
                    // x is two bytes long, and intrepreted as signed integer
                    x_coord_bytes += 2;
                }
            }

            if(flag & 0x4)
            {
                // y is one byte long, and the sign depends on the 0x20 bit
                y_coord_bytes++;
            }
            else
            {
                if(flag & 0x20)
                {
                    // same as prev
                }
                else
                {
                    // y is two bytes long, and intrepreted as signed integer
                    y_coord_bytes += 2;
                }
            }
        }

        u8 *x_coords = f;
        u8 *y_coords = f + x_coord_bytes;

        for(u32 flag_index = 0;
                flag_index <= flag_count;
                ++flag_index)
        {
            u8 flag = flags[flag_index];
        }
    }
    else
    {
        assert(number_of_contours == -1);
        // composite glyph description
    }
}



















