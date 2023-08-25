/**
 * @author : Vijaykumar Dangi
 * @date   : 24-Aug-2023
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include <cmath>

#include "../../../Common/FreeImage/x64/FreeImage.h"

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


struct Vector3
{
    float x, y, z;

    Vector3()
    {
        x = 0;
        y = 0;
        z = 0;
    }

    Vector3( float a, float b, float c)
    {
        x = a;
        y = b;
        z = c;
    }

    Vector3 operator+( Vector3& v)
    {
        return Vector3( x + v.x, y + v.y, z + v.z);
    }

    Vector3 operator-( Vector3& v)
    {
        return Vector3( x - v.x, y - v.y, z - v.z);
    }

    Vector3 operator*( Vector3& v)
    {
        return Vector3( x * v.x, y * v.y, z * v.z);
    }

    // Vector3 operator-()
    // {
    //     return Vector3( -x, -y, -z);
    // }

    Vector3 operator*( float scalar)
    {
        return Vector3( x * scalar, y * scalar, z * scalar);
    }

    Vector3 operator+( float scalar)
    {
        return Vector3( x + scalar, y + scalar, z + scalar);
    }

    Vector3 operator-( float scalar)
    {
        return Vector3( x - scalar, y - scalar, z - scalar);
    }
};

Vector3 operator*( float scalar, Vector3 v)
{
    return v * scalar;
}

Vector3 operator+( float scalar, Vector3 v)
{
    return v + scalar;
}

Vector3 operator-( float scalar, Vector3 v)
{
    return v - scalar;
}

/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // function declaration
    bool SaveImage( const char *out_file_name, uint8_t *image_data, int image_width, int image_height);
    uint8_t* LoadImage( const char *file_name, int *image_width, int *image_height);
    Vector3 GetTexel( uint8_t *image_bits, int image_width, int image_height, int x, int y);

    // variable declaration
    uint8_t *image_bits = nullptr;
    uint8_t *output_image_bits = nullptr;
    int image_width = 0;
    int image_height = 0;

    std::string input_image;

    // code
    for( int i = 1; i < argc; ++i)
    {
        std::string input( argv[i]);
        if( !input.compare( "--input"))
        {
            input_image = std::string( argv[++i]);
        }
    }

    if( input_image.empty())
    {
        std::cerr << "usage: " << argv[0] << " --input <input_image_name>\n";

        return EXIT_SUCCESS;
    }

        /******** IMAGE LOADING ***********/
    image_bits = LoadImage( input_image.c_str(), &image_width, &image_height);
    if( image_bits == nullptr)
    {
        std::cerr << "Cannot open image \"" << input_image << "\"" << std::endl;
        return EXIT_FAILURE;
    }

    output_image_bits = new uint8_t[ image_width * image_height * 4];

        /***************** STTART COMPUTATION **********************/

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();


    Vector3 p00, p10, p20;
    Vector3 p01,      p21;
    Vector3 p02, p12, p22;

    Vector3 Gx, Gy;
    Vector3 g;

    for( int j = 0; j < image_height; ++j)
    {
        for( int i = 0; i < image_width; ++i)
        {
            /*
            *          [p00  p10  p20]
            *          [p01  p11  p21]
            *          [p02  p12  p22]
            */
            
            p00 = GetTexel( image_bits, image_width, image_height, i - 1, j + 1);
            p10 = GetTexel( image_bits, image_width, image_height, i    , j + 1);
            p20 = GetTexel( image_bits, image_width, image_height, i + 1, j + 1);

            p01 = GetTexel( image_bits, image_width, image_height, i - 1, j    );
            p21 = GetTexel( image_bits, image_width, image_height, i + 1, j    );

            p02 = GetTexel( image_bits, image_width, image_height, i - 1, j - 1);
            p12 = GetTexel( image_bits, image_width, image_height, i    , j - 1);
            p22 = GetTexel( image_bits, image_width, image_height, i + 1, j - 1);

            /*
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
             * */

#if 0
            Gx = ( -1.0f * p00) + 0.0f + ( 1.0f * p20) +
                 ( -2.0f * p01) + 0.0f + ( 2.0f * p21) +
                 ( -1.0f * p02) + 0.0f + ( 1.0f * p22);

            Gy = (-1.0f * p00) + (-2.0f * p10) + (-1.0f * p20) +
                  0.0f +
                  (1.0f * p02) + (2.0f * p12) + (1.0f * p22);

            g = (Gx * Gx) + (Gy * Gy);

#else
            float Gxx = ( -1.0f * p00.x) + 0.0f + ( 1.0f * p20.x) +
                        ( -2.0f * p01.x) + 0.0f + ( 2.0f * p21.x) +
                        ( -1.0f * p02.x) + 0.0f + ( 1.0f * p22.x);

            float Gxy = ( -1.0f * p00.y) + 0.0f + ( 1.0f * p20.y) +
                        ( -2.0f * p01.y) + 0.0f + ( 2.0f * p21.y) +
                        ( -1.0f * p02.y) + 0.0f + ( 1.0f * p22.y);

            float Gxz = ( -1.0f * p00.z) + 0.0f + ( 1.0f * p20.z) +
                        ( -2.0f * p01.z) + 0.0f + ( 2.0f * p21.z) +
                        ( -1.0f * p02.z) + 0.0f + ( 1.0f * p22.z);

            float Gyx = (-1.0f * p00.x) + (-2.0f * p10.x) + (-1.0f * p20.x) +
                         0.0f +
                        ( 1.0f * p02.x) + ( 2.0f * p12.x) + ( 1.0f * p22.x);

            float Gyy = (-1.0f * p00.y) + (-2.0f * p10.y) + (-1.0f * p20.y) +
                         0.0f +
                        ( 1.0f * p02.y) + ( 2.0f * p12.y) + ( 1.0f * p22.y);

            float Gyz = (-1.0f * p00.z) + (-2.0f * p10.z) + (-1.0f * p20.z) +
                         0.0f +
                        ( 1.0f * p02.z) + ( 2.0f * p12.z) + ( 1.0f * p22.z);


            g.x = Gxx * Gxx + Gyx * Gyx;
            g.y = Gxy * Gxy + Gyy * Gyy;
            g.z = Gxz * Gxz + Gyz * Gyz;

#endif
            output_image_bits[ 4 * ( j * image_width + i) + 0] = (uint8_t) (sqrtf(g.z) * 255.0f);
            output_image_bits[ 4 * ( j * image_width + i) + 1] = (uint8_t) (sqrtf(g.y) * 255.0f);
            output_image_bits[ 4 * ( j * image_width + i) + 2] = (uint8_t) (sqrtf(g.x) * 255.0f);
            output_image_bits[ 4 * ( j * image_width + i) + 3] = 255;
        }
    }

    SaveImage( "out.png", output_image_bits, image_width, image_height);


    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double>  elapsed_seconds = end - start;
    std::cout << "Time Required for Histogram by CPU is: " << elapsed_seconds.count() << "s" << std::endl;

    if( output_image_bits)
    {
        delete output_image_bits;
        output_image_bits = nullptr;
    }

    if( image_bits)
    {
        delete image_bits;
        image_bits = nullptr;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief GetTexel() :
 */
Vector3 GetTexel( uint8_t *image_bits, int image_width, int image_height, int x, int y)
{
    // code
    x = std::min( std::max( x, 0), image_width - 1);
    y = std::min( std::max( y, 0), image_height - 1);

    int index = y * image_width + x;
    return Vector3( (float)image_bits[ 4 * index + 2] / 255.0f, (float)image_bits[ 4 * index + 1] / 255.0f, (float)image_bits[ 4 * index + 0] / 255.0f);
}


/**
 * @brief LoadImage() : Load Image and returns image width, image height and image data in 32-bit format. Delete image data when work is done.
 */
uint8_t* LoadImage( const char *file_name, int *image_width, int *image_height)
{
    // code
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType( file_name, 0);
    FIBITMAP *image = FreeImage_Load( format, file_name);
    if( image == nullptr)
    {
        *image_width = 0;
        *image_height = 0;
        return nullptr;
    }

        // convert to 32-bit image
    FIBITMAP *temp = image;
    image = FreeImage_ConvertTo32Bits( image);
    FreeImage_Unload( temp);

    *image_width = FreeImage_GetWidth( image);
    *image_height = FreeImage_GetHeight( image);

    uint8_t *image_bits = FreeImage_GetBits( image);

    uint8_t *ret_image_bits = new uint8_t[ (*image_width) * (*image_height) * 4];
    memcpy( ret_image_bits, image_bits, (*image_width) * (*image_height) * 4 * sizeof( uint8_t));

    FreeImage_Unload( image);

    return ret_image_bits;
}

/**
 * @brief SaveImage()
 */
bool SaveImage( const char *out_file_name, uint8_t *image_bits, int image_width, int image_height)
{
    // save image
    FREE_IMAGE_FORMAT format = FreeImage_GetFIFFromFilename( out_file_name);
    if( format == FREE_IMAGE_FORMAT::FIF_UNKNOWN)
    {
        return false;
    }

    int row_pitch = 4 * image_width;
    FIBITMAP *image = FreeImage_ConvertFromRawBits( image_bits, image_width, image_height, image_width * 4, 32, 0xFF000000, 0x00FF0000, 0x0000FF00);
    FreeImage_Save( format, image, out_file_name);
    FreeImage_Unload( image);

    return true;
}
