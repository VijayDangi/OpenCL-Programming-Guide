/**
 * @author : Vijaykumar Dangi
 * @date   : 24-Aug-2023
 */

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
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include "OpenCLUtil.h"

#include "../../../Common/FreeImage/x64/FreeImage.h"

#define To_String(x) #x

#define RELEASE_CL_OBJECT( obj, release_func) \
    if(obj) \
    {   \
        release_func(obj);    \
        obj = nullptr;  \
    }

const int OUT_IMAGE_WIDTH = 256;
const int OUT_IMAGE_HEIGHT = 1024;
const int BIN_COUNT = 256;
const int COLOR_RANGE = 256;

cl_context ocl_context = nullptr;
cl_command_queue ocl_command_queue = nullptr;
cl_device_id ocl_device = nullptr;
cl_program ocl_program = nullptr;

cl_kernel sobel_grayscale = nullptr;

cl_mem ocl_input_image = nullptr;
cl_mem ocl_output_image = nullptr;

uint8_t *image_bits = nullptr;
uint8_t *output_image_bits = nullptr;

/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // function declaration
    bool SaveImage( const char *out_file_name, uint8_t *image_data, int image_width, int image_height);
    uint8_t* LoadImage( const char *file_name, int *image_width, int *image_height);
    void  cleanup();

    // variable declaration
    int image_width = 0;
    int image_height = 0;

    std::string input_image;

    cl_int ocl_err;

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
        std::cerr << "options: " << "\n"
                  << "   -d: show dotted graph output\n"
                  << "   -s: separate output for each color channel"
                  << std::endl;

        return EXIT_SUCCESS;
    }

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

    ocl_program = CreateProgram( ocl_context, ocl_device, "sobel_grayscale.cl");
    if( ocl_program == nullptr)
    {
        std::cerr << "CreateProgram() Failed.";
        cleanup();
        return EXIT_FAILURE;
    }

    sobel_grayscale = clCreateKernel( ocl_program, "sobel_grayscale", &ocl_err);
    if( !ocl_program || ocl_err)
    {
        std::cerr << "clCreateKernel() Failed." << ocl_err << "\n";
        cleanup();
        return EXIT_FAILURE;
    }

        /******** IMAGE LOADING ***********/
    image_bits = LoadImage( input_image.c_str(), &image_width, &image_height);
    if( image_bits == nullptr)
    {
        std::cerr << "Cannot open image \"" << input_image << "\"" << std::endl;
        cleanup();
        return EXIT_FAILURE;
    }


        /********* COMPUTE HISTOGRAM ****************/
    size_t global_work_size[2];

    cl_image_format ocl_image_format = { };
    ocl_image_format.image_channel_order = CL_BGRA;
    ocl_image_format.image_channel_data_type = CL_UNORM_INT8;

    cl_image_desc ocl_image_desc = { };
    ocl_image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    ocl_image_desc.image_width = image_width;
    ocl_image_desc.image_height = image_height;
    ocl_image_desc.image_row_pitch = image_width * 4;
    ocl_image_desc.mem_object = nullptr;

        // input image
    ocl_input_image = clCreateImage( ocl_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &ocl_image_format, &ocl_image_desc, image_bits, &ocl_err);
    if( !ocl_input_image || ocl_err)
    {
        std::cerr << "clCreateImage() Failed." <<  ocl_err << "\n";
        cleanup();
        return EXIT_FAILURE;
    }

        // output image
    ocl_output_image = clCreateImage( ocl_context, CL_MEM_WRITE_ONLY, &ocl_image_format, &ocl_image_desc, nullptr, &ocl_err);
    if( !ocl_input_image || ocl_err)
    {
        std::cerr << "clCreateImage() Failed." <<  ocl_err << "\n";
        cleanup();
        return EXIT_FAILURE;
    }

    // ocl_sampler = clCreateSampler( ocl_context, CL_FALSE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST, &ocl_err);
    // if( !ocl_sampler || ocl_err)
    // {
    //     std::cerr << "clCreateSampler() Failed." <<  ocl_err << "\n";
    //     cleanup();
    //     return EXIT_FAILURE;
    // }

    global_work_size[0] = image_width;
    global_work_size[1] = image_height;

    std::cout << To_String( image_width) << " : " << image_width << "\n";
    std::cout << To_String( image_height) << " : " << image_height << "\n\n";

    std::cout << To_String( global_work_size[0]) << " : " << global_work_size[0] << "\n";
    std::cout << To_String( global_work_size[1]) << " : " << global_work_size[1] << "\n\n";


        //set kernel argument
    ocl_err = clSetKernelArg( sobel_grayscale, 0, sizeof( cl_mem), &ocl_input_image);
    ocl_err |= clSetKernelArg( sobel_grayscale, 1, sizeof( cl_mem), &ocl_output_image);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clSetKernelArg() Failed.\n";
        cleanup();
        return EXIT_FAILURE;
    }

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    ocl_err = clEnqueueNDRangeKernel( ocl_command_queue, sobel_grayscale, 2, nullptr, global_work_size, nullptr, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueNDRangeKernel() Failed.\n";
        cleanup();
        return EXIT_FAILURE;
    }

    //read result
    output_image_bits = new uint8_t[ image_width * image_height * 4];
    size_t origin[3] = { 0, 0, 0};
    size_t region[3] = { (size_t)image_width, (size_t)image_height, 1};
    size_t row_pitch = image_width * 4;

    ocl_err = clEnqueueReadImage( ocl_command_queue, ocl_output_image, CL_TRUE, origin, region, row_pitch, 0, output_image_bits, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueNDRangeKernel() Failed.\n";
        cleanup();
        return EXIT_FAILURE;
    }

    SaveImage( "Out.png", output_image_bits, image_width, image_height);


    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double>  elapsed_seconds = end - start;
    std::cout << "Time Required for Histogram by OpenCL is: " << elapsed_seconds.count() << "s" << std::endl;

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
    RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
    RELEASE_CL_OBJECT( sobel_grayscale, clReleaseKernel);
    RELEASE_CL_OBJECT( ocl_input_image, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_output_image, clReleaseMemObject);
    RELEASE_CL_OBJECT( image_bits, delete);
    RELEASE_CL_OBJECT( output_image_bits, delete);
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
