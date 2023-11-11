
const sampler_t sampler_ = CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

    ///////////////// Calculate Grayscale value of provided color
float getGrayScaleValue( float4 color)
{
    //return 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
    return (color.r + color.g + color.b) / 3.0;
}

    ///////////////// Subtract img_1 from img_0
__kernel void subtract_image(
    read_only image2d_t img_0,      // unsigned char RGBA
    read_only image2d_t img_1,      // unsigned char RGBA
    write_only image2d_t out_subtract_img,   // unsigned char RED
    int width,
    int height
)
{
    // code
    int x = get_global_id(0);
    int y = get_global_id(1);

    if( x >= width && y >= height)
    {
        return;
    }

    float val_0 = getGrayScaleValue( read_imagef( img_0, sampler_, (int2)(x, y)));
    float val_1 = getGrayScaleValue( read_imagef( img_1, sampler_, (int2)(x, y)));

    float diff = fabs( val_1 - val_0);
    // diff = (diff >= 0.05) ? diff : 0.0f;

    write_imagef( out_subtract_img, (int2)(x, y), (float4)( diff, 0.0, 0.0, 1.0f));
}

    /////////////////// Calculate Optic Flow Vector Map (Velocity)
__kernel void velocity_map(
    read_only image2d_t img_0,      // unsigned char RGBA
    read_only image2d_t img_1,      // unsigned char RGBA   
    read_only image2d_t subtract_img,   // unsigned char RED
    write_only image2d_t out_flow_vector,    //float RED_GREEN
    int search_size,
    int patch_size,
    int width,
    int height
)
{
    // code
    int x = get_global_id(0);
    int y = get_global_id(1);

    if( x >= width && y >= height)
    {
        return;
    }

    float patch_difference_max = INFINITY;

    // Search over a given rectangular area for a "patch" of old image
    // that "resembles" a patch of the new image.
    int half_search_size = search_size / 2;
    int half_patch_size = patch_size / 2;

    int sx, sy, px, py;
    int search_vector_x, search_vector_y;
    int patch_pixel_x, patch_pixel_y;
    int base_pixel_x, base_pixel_y;
    float patch_pixel, base_pixel;

    bool bUpdateFlowImage = false;
    float2 updated_flow_value = (float2)(0, 0);

    for( sy = -half_search_size; sy <= half_search_size; ++sy)
    {
        for( sx = -half_search_size; sx <= half_search_size; ++sx)
        {
            // searcg vector is center of patch test
            search_vector_x = x + sx;
            search_vector_y = y + sy;

            float accumulated_difference = 0.0;

            // For each pixel in search patch, accumulate difference with base patch
            for( py = -half_patch_size; py <= half_patch_size; ++py)
            {
                for( px = -half_patch_size; px <= half_patch_size; ++px)
                {
                    // Work out search patch offset indices
                    patch_pixel_x = search_vector_x + px;
                    patch_pixel_y = search_vector_y + py;

                    // Work out base patch indices
                    base_pixel_x = x + px;
                    base_pixel_y = y + py;

                    // Get adjaccent value for each patch
                    patch_pixel = getGrayScaleValue( read_imagef( img_1, sampler_, (int2)(patch_pixel_x, patch_pixel_y)));
                    base_pixel = getGrayScaleValue( read_imagef( img_0, sampler_, (int2)(base_pixel_x, base_pixel_y)));

                    // accumulated difference
                    accumulated_difference += fabs( patch_pixel - base_pixel);
                }
            }

            // Record the vector offset for the search patch that is
            // the least difference to the base patch
            if( accumulated_difference <= patch_difference_max)
            {
                bUpdateFlowImage = true;

                patch_difference_max = accumulated_difference;

                updated_flow_value.x = (float)( search_vector_x - x);
                updated_flow_value.y = (float)( search_vector_y - y);
            }
        }
    }

    if( bUpdateFlowImage)
    {
            // Modulate optic floaw vector map with motion map,
            // to remove vectors that erronously indicate large local motion.
        updated_flow_value *= (read_imagef( subtract_img, sampler_, (int2)(x, y)).r > 0) ? 1.0f : 0.0f;
        write_imagef( out_flow_vector, (int2)(x, y), (float4)( updated_flow_value, 0.0, 0.0));
    }
    else
    {
        write_imagef( out_flow_vector, (int2)(x, y), (float4)( 0.0, 0.0, 0.0, 0.0));
    }
}

    ////////////////// Draw Vector Lines
__kernel void vector_line(
    read_only image2d_t flow_vector_image,    //float RED_GREEN
    write_only image2d_t out_vector_image,  // unsigned char RED        // clear memory with 0 before calling kernel
    int search_size,
    int width, int height
)
{
    // code
    int x = get_global_id(0);
    int y = get_global_id(1);

    // float2 flow_vector = read_imagef( flow_vector_image, sampler_, (int2)(x, y)).rg;

    // write_imagef( out_vector_image, (int2)(x, y), (float4)( fabs(flow_vector.x), fabs(flow_vector.y), 0.0, 0.0));

    if( (x % search_size != search_size/2) || (y % search_size != search_size/2))
    {
        return;
    }

    int x1 = x;
    int y1 = y;

    float2 flow_vector = read_imagef( flow_vector_image, sampler_, (int2)(x, y)).rg;
    int dx = flow_vector.x;
    int dy = flow_vector.y;

    int len = ( dx >= dy) ? dx : dy;

    if( len != 0)
    {
        dx = dx / len;
        dy = dy / len;
    }

    int sx = ( dx >= 0) ? 1 : -1;
    int sy = ( dy >= 0) ? 1 : -1;

    float i = x1 + 0.5f * sx;
    float j = y1 + 0.5f * sy;

    int loop = 0;
    while( loop <= len)
    {
        if( (int)i >= 0 && (int)i < width && (int)j >= 0 && (int)j < height)
        {
            write_imagef( out_vector_image, (int2)(i, j), (float4)( 1.0, 0.0, 0.0, 0.0));
        }

        i = i + dx;
        j = j + dy;
        loop++;
    }
}
