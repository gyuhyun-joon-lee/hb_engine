kernel void add_arrays(device const float* inA,
                       device const float* inB,
                       device float* result,
                       uint index [[thread_position_in_threadgroup]])
{
    result[index] = inA[index] + inB[index];
}

