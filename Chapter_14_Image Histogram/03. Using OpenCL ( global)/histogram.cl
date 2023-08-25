#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

/**
 * @brief histogram_image_rgba_unorm8():
 *      This kernel takes an RGBA 8-bit-per-channel input image and produces a partial histogram
 * for R, G, and B.
 *
 */

__kernel void histogram_image_rgba_unorm8( read_only image2d_t img, sampler_t _sampler, __global uint *histogram)
{
    // variable declaration
    int x = get_global_id(0);
    int y = get_global_id(1);

    int image_width = get_image_width( img);
    int image_height = get_image_height( img);

    // code
    if( (x < image_width) && ( y < image_height))
    {
        float4 clr = read_imagef( img, _sampler, (float2)( x, y));

        uchar indx_x, indx_y, indx_z;

        //printf("(%3d, %3d) (%4d, %4d) : %f %f %f %f\n", x, y, image_width, image_height, clr.x, clr.y, clr.z, clr.w);
        indx_x = convert_uchar_sat( clr.x * 255.0f);
        indx_y = convert_uchar_sat( clr.y * 255.0f);
        indx_z = convert_uchar_sat( clr.z * 255.0f);

        atomic_inc( &histogram[ indx_x]);
        atomic_inc( &histogram[ 256 + (uint)indx_y]);
        atomic_inc( &histogram[ 512 + (uint)indx_z]);
    }
}

