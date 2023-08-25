/**
 * @author : Vijaykumar Dangi
 * @date   : 
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include "OpenCLUtil.h"

#include "../Common/FreeImage/x64/FreeImage.h"

#define To_String(x) #x

#define RELEASE_CL_OBJECT( obj, release_func) \
    if(obj) \
    {   \
        release_func(obj);    \
        obj = nullptr;  \
    }

cl_context ocl_context = nullptr;
cl_command_queue ocl_command_queue = nullptr;
cl_device_id ocl_device = nullptr;

/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // function declaration
    void  cleanup();

    // variable declaration
    cl_int ocl_err;

    // code
        // command line argument
    // for( int i = 1; i < argc; ++i)
    // {
    // }


        /******** Initialize OpenCL ***********/
    ocl_context = CreateContext( 0);
    if( ocl_context == nullptr)
    {
        std::cerr << "CreateContext() Failed.";
        cleanup();
        return EXIT_FAILURE;
    }

    ocl_command_queue = CreateCommandQueue( ocl_context, &ocl_device);
    if( ocl_command_queue == nullptr)
    {
        std::cerr << "CreateCommandQueue() Failed.";
        cleanup();
        return EXIT_FAILURE;
    }

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double>  elapsed_seconds = end - start;
    std::cout << "Time Required : " << elapsed_seconds.count() << "s" << std::endl;

    cleanup();

    return 0;
}

/**
 * @brief cleanup()
 */
void  cleanup()
{
    // code
    RELEASE_CL_OBJECT( ocl_context, clReleaseContext);
    RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
    RELEASE_CL_OBJECT( ocl_device, clReleaseDevice);
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
    FIBITMAP *image = FreeImage_ConvertFromRawBits( image_bits, image_width, image_height, row_pitch, 32, 0xFF000000, 0x00FF0000, 0x0000FF00);
    FreeImage_Save( format, image, out_file_name);
    FreeImage_Unload( image);

    return true;
}
