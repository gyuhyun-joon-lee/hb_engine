/*
 * Written by Gyuhyun Lee
 */
#include <stdio.h>

v3 gradient_vectors[] = 
{
    {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
    {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
    {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
};

// NOTE(gh) original 256 random numbers used by Ken Perlin, ranging from 0 to 255


/*
   NOTE(gh) This essentially picks a random vector from
    {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
    {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
    {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}

    and then dot product with the distance vector
*/
internal f32
get_gradient_value(i32 hash, f32 x, f32 y, f32 z)
{
    f32 result = 0.0f;

#if 1
    // only uses the first 4 bits
    switch(hash & 0xF)
    {
        case 0x0: result = x + y; break;
        case 0x1: result = -x + y; break;
        case 0x2: result =  x - y; break;
        case 0x3: result = -x - y; break;
        case 0x4: result =  x + z; break;
        case 0x5: result = -x + z; break;
        case 0x6: result =  x - z; break;
        case 0x7: result = -x - z; break;
        case 0x8: result =  y + z; break;
        case 0x9: result = -y + z; break;
        case 0xA: result = y - z; break;
        case 0xB: result = -y - z; break;
        case 0xC: result =  y + x; break;
        case 0xD: result = -y + z; break;
        case 0xE: result =  y - x; break;
        case 0xF: result = -y - z; break;
    }
#endif

    return result;
}

internal f32
fade(f32 t) 
{
    return t*t*t*(t*(t*6 - 15)+10);
}

internal f32 
perlin_noise01(u32 *permutations255, f32 x, f32 y, f32 z, u32 frequency)
{
    u32 xi = (u32)x;
    u32 yi = (u32)y;
    u32 zi = (u32)z;
    u32 x255 = xi % frequency; // used for hashing from the random numbers
    u32 y255 = yi % frequency; // used for hashing from the random numbers
    u32 z255 = zi % frequency;

    f32 xf = x - (f32)xi; // fraction part of x, should be in 0 to 1 range
    f32 yf = y - (f32)yi; // fraction part of y, should be in 0 to 1 range
    f32 zf = z - (f32)zi;

    f32 u = fade(xf);
    f32 v = fade(yf);
    f32 w = fade(zf);

    /*
       NOTE(gh) naming scheme
       2      3
       --------
       |      |
       |      |
       |      |
       --------
       0      1

       // +z
       6      7
       --------
       |      |
       |      |
       |      |
       --------
       4      5

       So for example, gradient vector 0 is a gradient vector used for the point 0, 
       and gradient 0 is the the graident value for point 0.
       The positions themselves do not matter that much, but the relative positions to each other
       are important as we need to interpolate in x, y, (and possibly z or w) directions.
    */

    // TODO(gh) We need to do this for now, because the perlin noise is per grid basis,
    // which means the edge of grid 0 should be using the same value as the start of grid 1.
    u32 x255_inc = (x255+1)%frequency;
    u32 y255_inc = (y255+1)%frequency;
    u32 z255_inc = (z255+1)%frequency;

    i32 random_value0 = permutations255[(permutations255[(permutations255[x255]+y255)%256]+z255)%256];
    i32 random_value1 = permutations255[(permutations255[(permutations255[x255_inc]+y255)%256]+z255)%256];
    i32 random_value2 = permutations255[(permutations255[(permutations255[x255]+y255_inc)%256]+z255)%256];
    i32 random_value3 = permutations255[(permutations255[(permutations255[x255_inc]+y255_inc)%256]+z255)%256];

    i32 random_value4 = permutations255[(permutations255[(permutations255[x255]+y255)%256]+z255_inc)%256];
    i32 random_value5 = permutations255[(permutations255[(permutations255[x255_inc]+y255)%256]+z255_inc)%256];
    i32 random_value6 = permutations255[(permutations255[(permutations255[x255]+y255_inc)%256]+z255_inc)%256];
    i32 random_value7 = permutations255[(permutations255[(permutations255[x255_inc]+y255_inc)%256]+z255_inc)%256];

    // NOTE(gh) -1 are for getting the distance vector
    f32 gradient0 = get_gradient_value(random_value0, xf, yf, zf);
    f32 gradient1 = get_gradient_value(random_value1, xf-1, yf, zf);
    f32 gradient2 = get_gradient_value(random_value2, xf, yf-1, zf);
    f32 gradient3 = get_gradient_value(random_value3, xf-1, yf-1, zf);

    f32 gradient4 = get_gradient_value(random_value4, xf, yf, zf-1);
    f32 gradient5 = get_gradient_value(random_value5, xf-1, yf, zf-1);
    f32 gradient6 = get_gradient_value(random_value6, xf, yf-1, zf-1);
    f32 gradient7 = get_gradient_value(random_value7, xf-1, yf-1, zf-1);

    f32 lerp01 = lerp(gradient0, u, gradient1); // lerp between 0 and 1
    f32 lerp23 = lerp(gradient2, u, gradient3); // lerp between 2 and 3 
    f32 lerp0123 = lerp(lerp01, v, lerp23); // lerp between '0-1' and '2-3', in y direction

    f32 lerp45 = lerp(gradient4, u, gradient5); // lerp between 4 and 5
    f32 lerp67 = lerp(gradient6, u, gradient7); // lerp between 6 and 7 
    f32 lerp4567 = lerp(lerp45, v, lerp67); // lerp between '4-5' and '6-7', in y direction

    f32 lerp01234567 = lerp(lerp0123, w, lerp4567);

    f32 result = (lerp01234567 + 1.0f) * 0.5f; // put into 0 to 1 range
    // f32 result = (lerp0123 + 1.0f) * 0.5f; // put into 0 to 1 range

    return result;
}

struct ThreadUpdatePerlinNoiseBufferData
{
    u32 total_x_count; 
    u32 total_y_count;

    u32 start_x;
    u32 start_y;

    u32 one_past_end_x;
    u32 one_past_end_y;

    u32 offset_x;
    u32 *permutations255;

    f32 time_elapsed_from_start;

    void *hash_buffer;
    void *perlin_noise_buffer;
};

internal
THREAD_WORK_CALLBACK(thread_update_perlin_noise_buffer_callback)
{
    ThreadUpdatePerlinNoiseBufferData *d = (ThreadUpdatePerlinNoiseBufferData *)data;

    // TODO(gh) Put this inside the game code, or maybe do this in compute shader
    u32 *hash_row = (u32 *)d->hash_buffer + d->start_y * d->total_x_count + d->start_x;
    f32 *row = (f32 *)d->perlin_noise_buffer + d->start_y * d->total_x_count + d->start_x;
    for(u32 y = d->start_y;
            y < d->one_past_end_y;
            ++y)
    {
        u32 *hash_column = (u32 *)hash_row;
        f32 *column = (f32 *)row;
        for(u32 x = d->start_x;
                x < d->one_past_end_x;
                ++x)
        {
            f32 xf = (x+d->offset_x) / (f32)d->total_x_count;
            f32 yf = y / (f32)d->total_y_count;

            // 
            u32 frequency = 16;

            f32 noise01 = perlin_noise01(d->permutations255, frequency*xf, frequency*yf, 2*d->time_elapsed_from_start, frequency);
            f32 wind_strength = 0.6f;
            f32 noise = wind_strength * (noise01 - 0.5f);

            *column++ = noise;
        }

        hash_row += d->total_x_count;
        row += d->total_x_count;
    }
}
