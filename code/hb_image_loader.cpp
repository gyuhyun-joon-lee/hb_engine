/*
 * Written by Gyuhyun Lee
 */

// TODO(joon): Move this into seperate header?
#pragma pack(push, 1)
struct bmp_file_header
{
    u16 file_header;
    u32 file_size;
    u16 reserved_1;
    u16 reserved_2;
    u32 pixel_offset;

    u32 header_size;
    u32 width;
    u32 height;
    u16 color_plane_count;
    u16 bits_per_pixel;
    u32 compression;

    u32 image_size;
    u32 pixels_in_meter_x;
    u32 pixels_in_meter_y;
    u32 colors;
    u32 important_color_count;
    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
    u32 alpha_mask;
};
#pragma pack(pop)

internal void
load_bmp(char *file_name)
{

}

internal void
export_bmp()
{
}

/*
    A PNG file starts with an 8-byte signature[13] (refer to hex editor image on the right):

    89	Has the high bit set to detect transmission systems that do not support 8-bit data and to reduce the chance that a text file is mistakenly interpreted as a PNG, or vice versa.
    50 4E 47	In ASCII, the letters PNG, allowing a person to identify the format easily if it is viewed in a text editor.
    0D 0A	A DOS-style line ending (CRLF) to detect DOS-Unix line ending conversion of the data.
    1A	 A byte that stops display of the file under DOS when the command type has been used—the end-of-file character.
    0A	A Unix-style line ending (LF) to detect Unix-DOS line ending conversion.

0	0x0D	13		IHDR chunk has 13 bytes of content
4	0x49484452		IHDR	Identifies a Header chunk
8	0x01	1		Image is 1 pixel wide
12	0x01	1		Image is 1 pixel high
16	0x08	8		8 bits per pixel (per channel)
17	0x02	2		Color type 2 (RGB/truecolor)
18	0x00	0		Compression method 0 (only accepted value)
19	0x00	0		Filter method 0 (only accepted value)
20	0x00	0		Not interlaced
21	0x907753DE			CRC of chunk's type and content (but not length)

he image's width (4 bytes); 
height (4 bytes); 
bit depth (1 byte, values 1, 2, 4, 8, or 16); 
color type (1 byte, values 0, 2, 3, 4, or 6); 
compression method (1 byte, value 0); 
filter method (1 byte, value 0); 
and interlace method (1 byte, values 0 "no interlace" or 1 "Adam7 interlace") (13 data bytes total)
*/

char png_signature[8] = {(char)137, 80, 78, 71, 13, 10, 26, 10};

// TODO(joon): png is big endian??
#pragma pack(push, 1)
struct png_header
{
    u32 width;
    u32 height;
    u8 bit_depth; // should be one of 1, 2, 4, 8, 16
    u8 color_type; // should be one of 0, 2, 3, 4, or 6 
    u8 compression; // should be 0
    u8 filter_method; // should be 0
    u8 interlace_method; // 0(no interlace) or 1(Adam7 interlace)
};
#pragma pack(pop)

// TODO(joon): Fill this up!
struct png_header_signature
{
    char c[8];
};

struct png_chunk_header
{
};

struct png_chunk_footer
{
};

// NOTE(joon): typical png chunk looks like : length / chunk name / chunk data / crc
// length is only for the 

internal void
check_png_chunk_header(char *c, const char *header_name)
{
    if(*c == header_name[0] && 
        *(c+1) == header_name[1] && 
        *(c+2) == header_name[2] && 
        *(c+3) == header_name[3])
    {
        c = c+4;
    }
}

internal void
load_png(PlatformReadFileResult file)
{
    u8 *c = (u8 *)file.memory;

    // NOTE(joon) : loop until we find IHDR file chunk, which is the start of png_header
    while(0)
    {

        c++;
    }
    u8 *width = (u8 *)(c+3);
    png_header *header = (png_header *)c;

    int a=  1;
}

internal void
export_png()
{
}

// TODO(joon) hdr support?




