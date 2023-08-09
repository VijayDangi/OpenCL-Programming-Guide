__kernel void square( __global int *buffer)
{
    //code
    int g_id = get_global_id(0);

    buffer[g_id] = buffer[g_id] * buffer[g_id];
}

