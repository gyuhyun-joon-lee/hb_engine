
// NOTE(joon): xorshift promises a huge speed boost comparing to rand(with the fact that it is really easy to use simd),
// but is not particulary well distributed(close to blue noise) rand function.
// TODO(joon): use PCG later?
internal void
xor_shift_32(u32 *state)
{
    u32 x = *state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x >> 5;

    *state = x;
}

struct RandomSeries
{
    u32 next_random;
};

internal RandomSeries
start_random_series(u32 seed)
{
    RandomSeries result = {};
    result.next_random = seed;

    return result;
}

internal f32
random_between_0_1(RandomSeries *series)
{
    xor_shift_32(&series->next_random);

    f32 result = (f32)series->next_random/(f32)U32_Max;
    return result;
}

internal r32
random_between(RandomSeries *series, r32 min, r32 max)
{
    xor_shift_32(&series->next_random);

    return min + (max-min)*random_between_0_1(series);
}

internal r32
random_between_minus_1_1(RandomSeries *series)
{
    xor_shift_32(&series->next_random);

    r32 result = 2.0f*((r32)series->next_random/(r32)U32_Max) - 1.0f;
    return result;
}

inline r32
random_range(RandomSeries *series, r32 value, r32 range)
{
    r32 half_range = range/2.0f;

    r32 result = value + half_range*random_between_minus_1_1(series);

    return result;
}

// TODO(joon) NOT truely random !!!
inline f32
random_f32(RandomSeries *series)
{
    xor_shift_32(&series->next_random);

    f32 result = (f32)series->next_random;

    return result;
}

inline u32
random_between_u32(RandomSeries *series, u32 min, u32 max)
{
    xor_shift_32(&series->next_random);
    
    return (u32)(series->next_random%(max-min) + min);
}

inline u32
random_u32(RandomSeries *series)
{
    xor_shift_32(&series->next_random);

    return series->next_random;
}

inline i32
random_between_i32(RandomSeries *series, i32 min, i32 max)
{
    xor_shift_32(&series->next_random);
    
    return (i32)series->next_random%(max-min) + min;
}

inline v3
random_normalized_v3(RandomSeries *series)
{
    v3 result = normalize(V3(random_f32(series), random_f32(series), random_f32(series)));
    return result;
}







