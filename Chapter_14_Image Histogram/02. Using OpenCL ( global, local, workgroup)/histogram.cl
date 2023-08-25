#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable

/**
 * @brief histogram_partial_image_rgba_unorm8():
 *      This kernel takes an RGBA 8-bit-per-channel input image and produces a partial histogram
 * for R, G, and B.
 * Each work-group represents an image tile and computes the histogram for that tile.
 *
 */

__kernel void histogram_partial_image_rgba_unorm8( image2d_t img, int num_pixels_per_workitem, __global uint *histogram)
{
    // variable declaration
    int local_size = (int)get_local_size(0) * (int)get_local_size(1);

    int image_width = get_image_width( img);
    int image_height = get_image_height( img);

    int group_indx = ( get_group_id(1) * get_num_groups(0) + get_group_id(0)) * 256 * 3;
    //int group_indx = mad24( (int)get_group_id(1), (int)get_num_groups(0), (int)get_group_id(0)) * 256 * 3;      // mad24( x, y, z) = (x * y) + z;

    int x = get_global_id(0);
    int y = get_global_id(1);

    local uint tmp_histogram[256 * 3];

    //int tid = get_local_id(1) * get_local_size(0) + get_local_id(0);
    int tid = mad24( (int)get_local_id(1), (int)get_local_size(0), (int)get_local_id(0));
    
    int j = 256 * 3;
    int indx = 0;

    // code
        //clear the local buffer that will generate the partial histogram
    do
    {
        if( tid < j)
        {
            tmp_histogram[ indx + tid] = 0;
        }

        j -= local_size;
        indx += local_size;
    } while( j > 0);

    barrier( CLK_LOCAL_MEM_FENCE);

    int i, idx;
    for( i = 0, idx = x; i < num_pixels_per_workitem; i++, idx += get_global_size(0))
    {
        if( (idx < image_width) && ( y < image_height))
        {
            float4 clr = read_imagef( img, 
                                CLK_NORMALIZED_COORDS_FALSE |
                                CLK_ADDRESS_CLAMP_TO_EDGE |
                                CLK_FILTER_NEAREST,
                                (float2)( idx/(float)image_width, y/(float)image_height)
                            );
        
            uchar indx_x, indx_y, indx_z;

            indx_x = convert_uchar_sat( clr.x * 255.0f);
            indx_y = convert_uchar_sat( clr.y * 255.0f);
            indx_z = convert_uchar_sat( clr.z * 255.0f);

            atomic_inc( &tmp_histogram[ indx_x]);
            atomic_inc( &tmp_histogram[ 256 + (uint)indx_y]);
            atomic_inc( &tmp_histogram[ 512 + (uint)indx_z]);
        }
    }
    
    barrier( CLK_LOCAL_MEM_FENCE);

        // copy the partial histogram to appropriate location in histogram given by group_indx
    if( local_size >= (256 * 3))
    {
        if( tid < ( 256 * 3))
        {
            histogram[ group_indx + tid] = tmp_histogram[ tid];
        }
    }
    else
    {
        j = 256 * 3;
        indx = 0;

        do
        {
            if( tid < j)
            {
                histogram[ group_indx + indx + tid] = tmp_histogram[ indx + tid];
            }

            j -= local_size;
            indx += local_size;
        }while( j > 0);
    }
}


/**
 * @brief histogram_sum_partial_results_unorm8():  This kernel sums partial histogram results into a final histogram result.
 *
 * @param num_groups is the number of work-groups used to compute partial histograms.
 *
 * @param partial_histogram is an array of num_groups * 256 * 3 entries, we store 256 R bins,
 *                  followed by 256 G bins, and then 256 B bins.
 *
 * The final summed results are returned in histogram.
 * 
 */
__kernel void histogram_sum_partial_results_unorm8( __global uint *partial_histogram, int num_groups, __global uint *histogram)
{
    // variable declaration
    int tid = ( int) get_global_id( 0);
    int group_indx;
    int n = num_groups;
    uint tmp_histogram;

    // code
    tmp_histogram = partial_histogram[tid];

    group_indx = 256 * 3;
    while( --n > 0)
    {
        tmp_histogram += partial_histogram[ group_indx + tid];
        group_indx += 256 * 3;
    }

    histogram[tid] = tmp_histogram;
}

