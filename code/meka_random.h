
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

struct random_series
{
    u32 next_random;
};

internal random_series
start_random_series(u32 seed)
{
    random_series result = {};
    result.next_random = seed;

    return result;
}

internal r32
random_between_0_1(random_series *series)
{
    xor_shift_32(&series->next_random);

    r32 result = (r32)series->next_random/(r32)u32_max;
    return result;
}

internal r32
random_between(random_series *series, r32 min, r32 max)
{
    xor_shift_32(&series->next_random);

    return min + (max-min)*random_between_0_1(series);
}

internal r32
random_between_minus_1_1(random_series *series)
{
    xor_shift_32(&series->next_random);

    r32 result = 2.0f*((r32)series->next_random/(r32)u32_max) - 1.0f;
    return result;
}

inline r32
random_range(random_series *series, r32 value, r32 range)
{
    r32 half_range = range/2.0f;

    r32 result = value + half_range*random_between_minus_1_1(series);

    return result;
}

inline u32
random_between_u32(random_series *series, u32 min, u32 max)
{
    xor_shift_32(&series->next_random);
    
    return series->next_random%(max-min) + min;
}







