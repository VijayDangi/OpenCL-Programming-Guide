__kernel void init_vbo_kernel( __global float2 *vbo, int w, int h, int seq)
{
    // code
    int g_id = get_global_id( 0);

    float2 line_pt;
    float freq = 4.0f;
    float amp = (float)h / 4.0f;
    float b = w / 2.0f;

    line_pt.x = (float)g_id / (float)w;
    line_pt.x = line_pt.x * 2.0f - 1.0f;

    line_pt.y = b + amp * sin( M_PI * 2.0 * ((float) g_id / (float)w * freq + (float)seq / (float)w ));
    line_pt.y /= (float)h;
    line_pt.y = line_pt.y * 2.0f - 1.0f;

    vbo[ g_id] = line_pt;
}

__kernel void init_texture_kernel( __write_only image2d_t img, int w, int h, int seq)
{
    // code
    int2 coord = { get_global_id(0), get_global_id(1) };

    float4 color = (float4)(
        (float)coord.x / (float)w,
        (float)coord.y / (float)h,
        (float)abs( seq - w) / (float) w,
        1.0
    );

    write_imagef( img, coord, color);
}

