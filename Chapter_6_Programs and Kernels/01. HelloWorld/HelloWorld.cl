__kernel void hello_kernel( __global const float *a,
                            __global const float *b,
                            __global float *result)
{
    //code
    int g_id = get_global_id(0);

    result[g_id] = a[g_id] + b[g_id];
}

