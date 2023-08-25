
/************************
 * 
 * The operator uses two 3x3 kernels which are convolved with the original image to compute derivatives,
 * one for horizontal changes and another for vertical.
 * 
 * Gx, the horizontal derivatives is,
 *  
 *               [ -1  0  +1]
 *          Gx = [ -2  0  +2]
 *               [ -1  0  +1]
 * 
 * Gy, the vertical derivatives is,
 *  
 *               [ -1  -2  -1]
 *          Gy = [  0   0   0]
 *               [ +1  +2  +1]
 * 
 * 
 * The gradient magnitude is computed as
 *      G = sqrt( Gx^2 + Gy^2)
 */

const sampler_t sampler_ = CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void sobel_edge_detection( read_only image2d_t src, write_only image2d_t dst)
{
    // code
    int x = ( int) get_global_id(0);
    int y = ( int) get_global_id(1);

    if( (x >= get_image_width(src)) || (y >= get_image_height(src)))
    {
        return;
    }

    /*
        [ p00  p10  p20]
        [ p01  p11  p21]
        [ p02  p12  p22]
     */

    float4 p00 = read_imagef( src, sampler_, (int2)( x - 1, y + 1));
    float4 p10 = read_imagef( src, sampler_, (int2)( x    , y + 1));
    float4 p20 = read_imagef( src, sampler_, (int2)( x + 1, y + 1));

    float4 p01 = read_imagef( src, sampler_, (int2)( x - 1, y    ));
    float4 p21 = read_imagef( src, sampler_, (int2)( x + 1, y    ));

    float4 p02 = read_imagef( src, sampler_, (int2)( x - 1, y - 1));
    float4 p12 = read_imagef( src, sampler_, (int2)( x    , y - 1));
    float4 p22 = read_imagef( src, sampler_, (int2)( x + 1, y - 1));


        // convole
    float3 Gx = ( -1.0f * p00.xyz) + 0.0f + ( 1.0f * p20.xyz) +
                ( -2.0f * p01.xyz) + 0.0f + ( 2.0f * p21.xyz) +
                ( -1.0f * p02.xyz) + 0.0f + ( 1.0f * p22.xyz);

    float3 Gy = ( -1.0f * p00.xyz) - (2.0f * p10.xyz) - (1.0f * p20.xyz) +
                0.0f +
                (  1.0f * p02.xyz) + (2.0f * p12.xyz) + (1.0f * p22.xyz);

    float3 g = native_sqrt( Gx * Gx + Gy * Gy);
    // float3 g = fabs( Gx) + fabs( Gy);

    write_imagef( dst, (int2)(x, y), (float4)( g.x, g.y, g.z, 1.0f));
}
