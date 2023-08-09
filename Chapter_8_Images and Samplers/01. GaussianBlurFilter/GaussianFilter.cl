__kernel void gaussian_filter(
    __read_only image2d_t src_imag,
    __write_only image2d_t dst_img,
    sampler_t _sampler,
    int width,
    int height
)
{
    // Gaussian Kernel is
    //
    //  1  2  1
    //  2  4  2
    //  1  2  1
    //
    float kernel_weights[] = {
        1.0f, 2.0f, 1.0f,
        2.0f, 4.0f, 2.0f,
        1.0f, 2.0f, 1.0f
    };

    // float kernel_weights[] = {
    //     4.0f, 2.0f, 4.0f,
    //     2.0f, 1.0f, 2.0f,
    //     4.0f, 2.0f, 4.0f
    // };

    // float kernel_weights[] = {
    //     4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f,
    //     0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f,
    //     0.0f, 0.0f, 4.0f, 0.0f, 4.0f, 0.0f, 0.0f,
    //     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    //     0.0f, 0.0f, 4.0f, 0.0f, 4.0f, 0.0f, 0.0f,
    //     0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f,
    //     4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f
    // };

    int count = 0;
    int num_kernel_element = sizeof(kernel_weights) / sizeof(kernel_weights[0]);

    for( int i = 0; i < num_kernel_element; ++i)
    {
        count += (int)kernel_weights[i];
    }

    size_t x = get_global_id( 0);
    size_t y = get_global_id( 1);

    int loop_count = (int)sqrt((float)num_kernel_element);
    loop_count = loop_count / 2;

    int2 start_image_coord = (int2) ( x - loop_count, y - loop_count);
    int2 end_image_coord = (int2) ( x + loop_count, y + loop_count);
    int2 out_image_coord = (int2) ( x, y);

    if( (out_image_coord.x < width) && (out_image_coord.y < height))
    {
        int weight = 0;
        float4 out_color = (float4)( 0.0f, 0.0f, 0.0f, 0.0f);

        for( int j = start_image_coord.y; j <= end_image_coord.y; ++j)
        {
            for( int i = start_image_coord.x; i <= end_image_coord.x; ++i)
            {
                out_color += ( read_imagef( src_imag, _sampler, (int2)(i, j)) * kernel_weights[weight] / (float)count);
                ++weight;
            }
        }

        // Write the output value to image
        write_imagef( dst_img, out_image_coord, out_color);
    }
} 

