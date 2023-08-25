/**
 * @author : Vijaykumar Dangi
 * @date   : 19-Aug-2023
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include "OpenCLUtil.h"

#include "../../Common/FreeImage/x64/FreeImage.h"

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
cl_kernel fn_histogram_partial_image_rgba_unorm8 = nullptr;
cl_kernel fn_histogram_sum_partial_results_unorm8 = nullptr;

cl_mem ocl_histogram_buffer = nullptr;
cl_mem ocl_partial_histogram_buffer = nullptr;
cl_mem ocl_input_image = nullptr;

unsigned int *histogram_result = nullptr;
uint8_t *image_bits = nullptr;

const int num_pixels_per_work_item = 32;

/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // function declaration
    bool SaveHistogramGraphImage( uint32_t *red_channel_data, uint32_t *green_channel_data, uint32_t *blue_channel_data, bool b_filled_graph, bool b_sepated_output);
    uint8_t* LoadImage( const char *file_name, int *image_width, int *image_height);
    void  cleanup();

    // variable declaration
    int image_width = 0;
    int image_height = 0;

    bool b_save_filled_graph = true;
    bool b_save_separate_channel_graph = false;

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
        else if( !input.compare( "-d"))
        {
            b_save_filled_graph = false;
        }
        else if( !input.compare( "-s"))
        {
            b_save_separate_channel_graph = true;
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

    ocl_program = CreateProgram( ocl_context, ocl_device, "histogram.cl");
    if( ocl_program == nullptr)
    {
        std::cerr << "CreateProgram() Failed.";
        cleanup();
        return EXIT_FAILURE;
    }

    fn_histogram_sum_partial_results_unorm8 = clCreateKernel( ocl_program, "histogram_sum_partial_results_unorm8", &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clCreateKernel() Failed.";
        cleanup();
        return EXIT_FAILURE;
    }

    fn_histogram_partial_image_rgba_unorm8 = clCreateKernel( ocl_program, "histogram_partial_image_rgba_unorm8", &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clCreateKernel() Failed.";
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
    size_t workgroup_size;
    size_t global_work_size[2];
    size_t local_work_size[2];

    size_t partial_global_work_size[1];
    size_t partial_local_work_size[1];

    size_t num_groups;

        // result histogram buffer
    ocl_histogram_buffer = clCreateBuffer( ocl_context, CL_MEM_WRITE_ONLY, COLOR_RANGE * 3 * sizeof( unsigned int), nullptr, &ocl_err);
    if( !ocl_histogram_buffer || ocl_err)
    {
        std::cerr << "clCeateBuffer() Failed.\n";
        cleanup();
        return EXIT_FAILURE;
    }

    cl_image_format ocl_image_format;
    ocl_image_format.image_channel_order = CL_BGRA;
    ocl_image_format.image_channel_data_type = CL_UNORM_INT8;

        // input image
    ocl_input_image = clCreateImage2D(
                        ocl_context,
                        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                        &ocl_image_format,
                        image_width, image_height, 0, image_bits, &ocl_err
                    );
    if( !ocl_input_image || ocl_err)
    {
        std::cerr << "clCreateImage2D() Failed.\n";
        cleanup();
        return EXIT_FAILURE;
    }

        // kernel execution
    clGetKernelWorkGroupInfo(
        fn_histogram_partial_image_rgba_unorm8,
        ocl_device, CL_KERNEL_WORK_GROUP_SIZE, sizeof( size_t), &workgroup_size, nullptr);

    size_t g_size[2];
    int w;

    if( workgroup_size <= 256)
    {
        g_size[0] = 16;
        g_size[1] = workgroup_size / 16;
    }
    else if( workgroup_size <= 1024)
    {
        g_size[0] = workgroup_size / 16;
        g_size[1] = 16;
    }
    else
    {
        g_size[0] = workgroup_size / 32;
        g_size[1] = 32;
    }

    local_work_size[0] = g_size[0];
    local_work_size[1] = g_size[1];

    w = ( image_width + num_pixels_per_work_item - 1) / num_pixels_per_work_item;
    global_work_size[0] = ( w + g_size[0] - 1) / g_size[0];
    global_work_size[1] = ( image_height + g_size[1] - 1) / g_size[1];

    num_groups = global_work_size[0] * global_work_size[1];
    global_work_size[0] *= g_size[0];
    global_work_size[1] *= g_size[1];

    std::cout << "CL_KERNEL_WORK_GROUP_SIZE: " << workgroup_size << "\n\n";

    std::cout << To_String( image_width) << " : " << image_width << "\n";
    std::cout << To_String( image_height) << " : " << image_height << "\n\n";

    std::cout << To_String( g_size[0]) << " : " << g_size[0] << "\n";
    std::cout << To_String( g_size[1]) << " : " << g_size[1] << "\n\n";

    std::cout << To_String( local_work_size[0]) << " : " << local_work_size[0] << "\n";
    std::cout << To_String( local_work_size[1]) << " : " << local_work_size[1] << "\n\n";

    std::cout << To_String( num_groups) << " : " << num_groups << "\n\n";

    std::cout << To_String( global_work_size[0]) << " : " << global_work_size[0] << "\n";
    std::cout << To_String( global_work_size[1]) << " : " << global_work_size[1] << "\n\n";


    ocl_partial_histogram_buffer = clCreateBuffer(
                                        ocl_context,
                                        CL_MEM_READ_WRITE,
                                        num_groups * COLOR_RANGE * 3 * sizeof( unsigned int), nullptr, &ocl_err);
    if( !ocl_partial_histogram_buffer || ocl_err)
    {
        std::cerr << "clCreateBuffer() failed.\n";
        cleanup();
        return EXIT_FAILURE;
    }


    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        // set parameter for histogram_partial_image_rgba_unorm8() kernel
    ocl_err = clSetKernelArg( fn_histogram_partial_image_rgba_unorm8, 0, sizeof( cl_mem), &ocl_input_image);
    ocl_err |= clSetKernelArg( fn_histogram_partial_image_rgba_unorm8, 1, sizeof( int), &num_pixels_per_work_item);
    ocl_err |= clSetKernelArg( fn_histogram_partial_image_rgba_unorm8, 2, sizeof( cl_mem), &ocl_partial_histogram_buffer);
    if( ocl_err)
    {
        std::cerr << "clSetKernelArg() Failed." << ocl_err << "\n";
        cleanup();
        return EXIT_FAILURE;
    }


        // set parameter for histogram_sum_partial_results_unorm8() kernel
    ocl_err = clSetKernelArg( fn_histogram_sum_partial_results_unorm8, 0, sizeof( cl_mem), &ocl_partial_histogram_buffer);
    ocl_err |= clSetKernelArg( fn_histogram_sum_partial_results_unorm8, 1, sizeof( int), &num_groups);
    ocl_err |= clSetKernelArg( fn_histogram_sum_partial_results_unorm8, 2, sizeof( cl_mem), &ocl_histogram_buffer);
    if( ocl_err)
    {
        std::cerr << "clSetKernelArg() Failed." << ocl_err << "\n";
        cleanup();
        return EXIT_FAILURE;
    }

        // execute kernel
    ocl_err = clEnqueueNDRangeKernel(
                ocl_command_queue,
                fn_histogram_partial_image_rgba_unorm8,
                2, nullptr, global_work_size, local_work_size, 0, nullptr, nullptr);
    if( ocl_err)
    {
        std::cerr << "clEnqueueNDRangeKernel() Failed." << ocl_err << "\n";
        cleanup();
        return EXIT_FAILURE;
    }


    clGetKernelWorkGroupInfo( fn_histogram_sum_partial_results_unorm8, ocl_device, CL_KERNEL_WORK_GROUP_SIZE, sizeof( size_t), &workgroup_size, nullptr);
    std::cout << "CL_KERNEL_WORK_GROUP_SIZE: " << workgroup_size << "\n\n";
    if( workgroup_size < 256)
    {
        std::cerr << "A minimum of 256 work-items in work-group needed for histogram_sum_partial_results_unorm8() kernel. \n";
        cleanup();
        return EXIT_FAILURE;
    }

    partial_global_work_size[0] = 256 * 3;
    partial_local_work_size[0] = ( workgroup_size > 256) ? 256 : workgroup_size;

    std::cout << To_String( partial_global_work_size[0]) << " : " << partial_global_work_size[0] << "\n";
    std::cout << To_String( partial_local_work_size[0]) << " : " << partial_local_work_size[0] << "\n";

    ocl_err = clEnqueueNDRangeKernel(
                ocl_command_queue,
                fn_histogram_sum_partial_results_unorm8,
                1, nullptr, partial_global_work_size, partial_local_work_size, 0, nullptr, nullptr);
    if( ocl_err)
    {
        std::cerr << "clEnqueueNDRangeKernel() Failed." << ocl_err << "\n";
        cleanup();
        return EXIT_FAILURE;
    }

        // read the result
    histogram_result = new unsigned int[COLOR_RANGE * 3];
    if( histogram_result == nullptr)
    {
        std::cerr << "Memory Allocation Failed.\n";
        cleanup();
        return EXIT_FAILURE;
    }

    ocl_err = clEnqueueReadBuffer( ocl_command_queue, ocl_histogram_buffer, CL_TRUE, 0, COLOR_RANGE * 3 * sizeof(unsigned int), histogram_result, 0, nullptr, nullptr);
    if( ocl_err)
    {
        std::cerr << "clEnqueueReadBuffer() Failed." << ocl_err << "\n";
        cleanup();
        return EXIT_FAILURE;
    }

        /******** SAVE HISTOGRAM *******************/
    SaveHistogramGraphImage( histogram_result, histogram_result + 256 , histogram_result + 512 , b_save_filled_graph, b_save_separate_channel_graph);


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
    RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
    RELEASE_CL_OBJECT( fn_histogram_partial_image_rgba_unorm8, clReleaseKernel);
    RELEASE_CL_OBJECT( fn_histogram_sum_partial_results_unorm8, clReleaseKernel);
    
    RELEASE_CL_OBJECT( ocl_histogram_buffer, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_partial_histogram_buffer, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_input_image, clReleaseMemObject);

    RELEASE_CL_OBJECT( histogram_result, delete);
    RELEASE_CL_OBJECT( image_bits, delete);
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
 * @brief SaveHistogramGraphImage()
 */
bool SaveHistogramGraphImage( uint32_t *red_channel_data, uint32_t *green_channel_data, uint32_t *blue_channel_data, bool b_filled_graph, bool b_sepated_output)
{
    // function declaration
    bool SaveHistogramImage( uint32_t *red_data, uint32_t *green_data, uint32_t *blue_data, bool b_filled_graph);
    bool SaveSeparateHistogramImage( uint32_t *red_data, uint32_t *green_data, uint32_t *blue_data, bool b_filled_graph);

    // variable declaration
    int max_red = 0;
    int max_green = 0;
    int max_blue = 0;

    bool result = true;

    // code
        // normalize image [0 - 256]
    for( int i = 0; i < BIN_COUNT; ++i)
    {
        if( max_red < red_channel_data[i])
        {
            max_red = red_channel_data[i];
        }

        if( max_green < green_channel_data[i])
        {
            max_green = green_channel_data[i];
        }

        if( max_blue < blue_channel_data[i])
        {
            max_blue = blue_channel_data[i];
        }
    }

    for( int i = 0; i < BIN_COUNT; ++i)
    {
        red_channel_data[i]   = ( (float)red_channel_data[i] / (float)max_red) * (OUT_IMAGE_HEIGHT - 1);
        green_channel_data[i] = ( (float)green_channel_data[i] / (float)max_green) * (OUT_IMAGE_HEIGHT - 1);
        blue_channel_data[i]  = ( (float)blue_channel_data[i] / (float)max_blue) * (OUT_IMAGE_HEIGHT - 1);
    }


    if( b_sepated_output)
    {
        result = SaveSeparateHistogramImage( red_channel_data, green_channel_data, blue_channel_data, b_filled_graph);
    }
    else
    {
        result = SaveHistogramImage( red_channel_data, green_channel_data, blue_channel_data, b_filled_graph);
    }


    return result;
}

/**
 * @brief SaveHistogramImage()
 */
bool SaveHistogramImage( uint32_t *red_data, uint32_t *green_data, uint32_t *blue_data, bool b_filled_graph)
{
    // variable declaration
    uint8_t *image_buffer = nullptr;

    // code
    image_buffer = new uint8_t[ OUT_IMAGE_WIDTH * OUT_IMAGE_HEIGHT * 3]();

    for( int i = 0; i < BIN_COUNT; ++i)
    {
        int j;

        if( b_filled_graph)
        {
            // red
            for( j = 0; j < red_data[i]; ++j)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 2] = 0xFF;
            }

            // green
            for( j = 0; j < green_data[i]; ++j)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 1] = 0xFF;
            }

            // blue
            for( j = 0; j < blue_data[i]; ++j)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 0] = 0xFF;
            }
        }
        else
        {
            // red
            j = red_data[i] - 1;
            if( j >= 0)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 2] = 0xFF;
            }

            // green
            j = green_data[i] - 1;
            if( j >= 0)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 1] = 0xFF;
            }

            // blue
            j = blue_data[i] - 1;
            if( j >= 0)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 0] = 0xFF;
            }
        }
    }

    // save image
    std::string out_image = "out.png";
    FREE_IMAGE_FORMAT format = FreeImage_GetFIFFromFilename( out_image.c_str());
    int row_pitch = 3 * OUT_IMAGE_WIDTH;
    FIBITMAP *image = FreeImage_ConvertFromRawBits( image_buffer, OUT_IMAGE_WIDTH, OUT_IMAGE_HEIGHT, row_pitch, 24, 0xFF000000, 0x00FF0000, 0x0000FF00);
    FreeImage_Save( format, image, out_image.c_str());
    FreeImage_Unload( image);

    delete image_buffer;
    image_buffer = nullptr;

    return true;
}

/**
 * @brief SaveSeparateHistogramImage()
 */
bool SaveSeparateHistogramImage( uint32_t *red_data, uint32_t *green_data, uint32_t *blue_data, bool b_filled_graph)
{
    // variable declaration
    uint8_t *image_red_buffer = nullptr;
    uint8_t *image_green_buffer = nullptr;
    uint8_t *image_blue_buffer = nullptr;

    // code
    image_red_buffer   = new uint8_t[ OUT_IMAGE_WIDTH * OUT_IMAGE_HEIGHT * 3]();
    image_green_buffer = new uint8_t[ OUT_IMAGE_WIDTH * OUT_IMAGE_HEIGHT * 3]();
    image_blue_buffer  = new uint8_t[ OUT_IMAGE_WIDTH * OUT_IMAGE_HEIGHT * 3]();

    for( int i = 0; i < BIN_COUNT; ++i)
    {
        int j;

        if( b_filled_graph)
        {
            // red
            for( j = 0; j < red_data[i]; ++j)
            {
                image_red_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 2] = 0xFF;
            }

            // green
            for( j = 0; j < green_data[i]; ++j)
            {
                image_green_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 1] = 0xFF;
            }

            // blue
            for( j = 0; j < blue_data[i]; ++j)
            {
                image_blue_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 0] = 0xFF;
            }
        }
        else
        {
            // red
            j = red_data[i] - 1;
            if( j >= 0)
            {
                image_red_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 2] = 0xFF;
            }

            // green
            j = green_data[i] - 1;
            if( j >= 0)
            {
                image_green_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 1] = 0xFF;
            }

            // blue
            j = blue_data[i] - 1;
            if( j >= 0)
            {
                image_blue_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 0] = 0xFF;
            }
        }
    }

    // save images
    {
        std::string out_image = "out_red.png";
        FREE_IMAGE_FORMAT format = FreeImage_GetFIFFromFilename( out_image.c_str());
        int row_pitch = 3 * OUT_IMAGE_WIDTH;
        FIBITMAP *image = FreeImage_ConvertFromRawBits( image_red_buffer, OUT_IMAGE_WIDTH, OUT_IMAGE_HEIGHT, row_pitch, 24, 0xFF000000, 0x00FF0000, 0x0000FF00);
        FreeImage_Save( format, image, out_image.c_str());
        FreeImage_Unload( image);
    }

    {
        std::string out_image = "out_green.png";
        FREE_IMAGE_FORMAT format = FreeImage_GetFIFFromFilename( out_image.c_str());
        int row_pitch = 3 * OUT_IMAGE_WIDTH;
        FIBITMAP *image = FreeImage_ConvertFromRawBits( image_green_buffer, OUT_IMAGE_WIDTH, OUT_IMAGE_HEIGHT, row_pitch, 24, 0xFF000000, 0x00FF0000, 0x0000FF00);
        FreeImage_Save( format, image, out_image.c_str());
        FreeImage_Unload( image);
    }

    {
        std::string out_image = "out_blue.png";
        FREE_IMAGE_FORMAT format = FreeImage_GetFIFFromFilename( out_image.c_str());
        int row_pitch = 3 * OUT_IMAGE_WIDTH;
        FIBITMAP *image = FreeImage_ConvertFromRawBits( image_blue_buffer, OUT_IMAGE_WIDTH, OUT_IMAGE_HEIGHT, row_pitch, 24, 0xFF000000, 0x00FF0000, 0x0000FF00);
        FreeImage_Save( format, image, out_image.c_str());
        FreeImage_Unload( image);
    }

    delete image_red_buffer;
    image_red_buffer = nullptr;

    delete image_green_buffer;
    image_green_buffer = nullptr;

    delete image_blue_buffer;
    image_blue_buffer = nullptr;

    return true;
}
