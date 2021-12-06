// TODO(joon) : make this to use random table

internal r32
random_between_0_1()
{
    r32 result = (r32)rand()/(r32)RAND_MAX;
    return result;
}

internal r32
random_between(r32 min, r32 max)
{
    return min + (max-min)*random_between_0_1();
}

internal r32
random_between_minus_1_1()
{
    r32 result = 2.0f*((r32)rand()/(r32)RAND_MAX) - 1.0f;
    return result;
}

inline r32
random_range(r32 value, r32 range)
{
    r32 half_range = range/2.0f;

    r32 result = value + half_range*random_between_minus_1_1();

    return result;
}
