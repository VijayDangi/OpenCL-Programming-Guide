/**
 * @author : Vijaykumar Dangi
 * @date   : 3-August-2023
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include <CL/cl.h>

#include "Info.hpp"
#include "../../Common/FreeImage/x64/FreeImage.h"


#define DEFAULT_PLATFORM 0
#define DEFAULT_USE_MAP false

#define RELEASE_CL_OBJECT( obj, release_func) \
    if(obj) \
    {   \
        release_func(obj);    \
        obj = nullptr;  \
    }

/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // function declaration
    cl_context CreateContext( int platform_used);
    cl_command_queue CreateCommandQueue( cl_context, cl_device_id* );
    cl_program CreateProgram( cl_context, cl_device_id, const char* );
    cl_mem LoadImage( cl_context ocl_context, const char *file_name, int &width, int &height);
    bool SaveImage( const char *file_name, char *buffer, int image_width, int image_height, int row_pitch);
    size_t RoundUp( int group_size, int global_size);

    // variable declaration
    cl_context ocl_context = nullptr;
    cl_program ocl_program = nullptr;
    cl_command_queue ocl_command_queue = nullptr;
    cl_kernel ocl_kernel = nullptr;

    cl_mem ocl_image_src = nullptr;
    cl_mem ocl_image_dst = nullptr;
    cl_sampler ocl_sampler = nullptr;

    int image_width = 0;
    int image_height = 0;

    cl_int ocl_err = 0;
    cl_uint num_platforms = 0;

    int platform_used = DEFAULT_PLATFORM;
    bool b_use_map_buffer = DEFAULT_USE_MAP;

    std::string output_file_name;
    std::string input_file_name;

    // code
    if( argc < 3)
    {
        std::cout << "usage: " << argv[0] << " --i input_file_name --platform n --useMapBuffer --o output_file_name" << std::endl;

        return 0;
    }


    for( int i = 1; i < argc; ++i)
    {
        std::string input( argv[i]);

        if( !input.compare( "--platform"))
        {
            input = std::string( argv[++i]);
            std::istringstream buffer(input);
            buffer >> platform_used;
        }
        else if( !input.compare("--useMapBuffer"))
        {
            b_use_map_buffer = true;
        }
        else if( !input.compare( "--o"))
        {
            output_file_name = std::string( argv[++i]);
        }
        else if( !input.compare( "--i"))
        {
            input_file_name = std::string( argv[++i]);
        }
        else
        {
            std::cout << "usage: %s --i input_file_name --platform n --useMapBuffer --o output_file_name" << std::endl;
            return 0;
        }
    }

    if( input_file_name.empty())
    {
        std::cerr << "Input file name not provided." << std::endl;
        std::cout << "usage: %s --i input_file_name --platform n --useMapBuffer --o output_file_name" << std::endl;
        return 0;
    }
    
    if( output_file_name.empty())
    {
        std::cerr << "Output file name not provided." << std::endl;
        std::cout << "usage: %s --i input_file_name --platform n --useMapBuffer --o output_file_name" << std::endl;
        return 0;
    }


        // Create an OpenCL context on first available platform
    ocl_context = CreateContext( platform_used);
    if( ocl_context == nullptr)
    {
        std::cerr << "Failed to create OpenCL Context." << std::endl;
        return 1;
    }


        // Get Devices
    size_t num_devices = 0;
    ocl_err = clGetContextInfo( ocl_context, CL_CONTEXT_NUM_DEVICES, sizeof( cl_uint), &num_devices, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clGetContextInfo() Failed." << __LINE__ << std::endl;
        RELEASE_CL_OBJECT( ocl_context, clReleaseContext);
        return 1;
    }


    cl_device_id *ocl_devices = new cl_device_id[num_devices];
    ocl_err = clGetContextInfo( ocl_context, CL_CONTEXT_DEVICES, num_devices * sizeof( cl_device_id), ocl_devices, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clGetContextInfo() Failed." << __LINE__ << std::endl;
        RELEASE_CL_OBJECT( ocl_context, clReleaseContext);
        return 1;
    }


        // Make Sure the device supports images
    cl_bool image_support = CL_FALSE;

    for( int i = 0; i < num_devices; ++i)
    {
        image_support = CL_FALSE;
        clGetDeviceInfo( ocl_devices[i], CL_DEVICE_IMAGE_SUPPORT, sizeof( cl_bool), &image_support, nullptr);
        if( image_support != CL_TRUE)
        {
            std::cerr << "OpenCL device does not support images." << std::endl;

            delete ocl_devices;
            ocl_devices = nullptr;

            RELEASE_CL_OBJECT( ocl_context, clReleaseContext);
            return 1;
        }
    }


        // create command queue
    ocl_command_queue = clCreateCommandQueue( ocl_context, ocl_devices[0], 0, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Failed to create Command Queue." << std::endl;

        delete ocl_devices;
        ocl_devices = nullptr;

        RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

        return 1;
    }


    delete ocl_devices;
    ocl_devices = nullptr;


        // Create program from source
    ocl_program = CreateProgram( ocl_context, nullptr, "GaussianFilter.cl");
    if( ocl_program == nullptr)
    {
        std::cerr << "CreateProgram() Failed." << __LINE__ << std::endl;

        RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
        RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

        return 1;
    }


        // Create kernel object
    ocl_kernel = clCreateKernel( ocl_program, "gaussian_filter", &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clCreateKernel() Failed." << std::endl;

        RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
        RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
        RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

        return 1;
    }


        // load source image
    ocl_image_src = LoadImage( ocl_context, input_file_name.c_str(), image_width, image_height);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "LoadImage() Failed." << std::endl;

        RELEASE_CL_OBJECT( ocl_kernel, clReleaseKernel);
        RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
        RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
        RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

        return 1;
    }


        // destination image
        // Cretae output image object
    cl_image_format ocl_image_format;
    ocl_image_format.image_channel_order = CL_RGBA;
    ocl_image_format.image_channel_data_type = CL_UNORM_INT8;

    ocl_image_dst = clCreateImage2D( ocl_context, CL_MEM_WRITE_ONLY, &ocl_image_format, image_width, image_height, 0, nullptr, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clCreateImage2D() Failed ( " << __LINE__ << " )." << std::endl;

        RELEASE_CL_OBJECT( ocl_image_src, clReleaseMemObject);
        RELEASE_CL_OBJECT( ocl_kernel, clReleaseKernel);
        RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
        RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
        RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

        return 1;
    }


        // Create sampler for sampling image object
        //
        //  can create sampler in kernel as:
        //
        //  const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
        //
    ocl_sampler = clCreateSampler(
                        ocl_context,
                        CL_FALSE,   // Non-normalized coordinates
                        CL_ADDRESS_CLAMP_TO_EDGE,
                        CL_FILTER_NEAREST,
                        &ocl_err
                    );
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error creating CL sampler object." << std::endl;

        RELEASE_CL_OBJECT( ocl_image_dst, clReleaseMemObject);
        RELEASE_CL_OBJECT( ocl_image_src, clReleaseMemObject);
        RELEASE_CL_OBJECT( ocl_kernel, clReleaseKernel);
        RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
        RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
        RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

        return 1;
    }


        // Set the kernel argument
    ocl_err = clSetKernelArg( ocl_kernel, 0, sizeof( cl_mem), &ocl_image_src);
    ocl_err |= clSetKernelArg( ocl_kernel, 1, sizeof( cl_mem), &ocl_image_dst);
    ocl_err |= clSetKernelArg( ocl_kernel, 2, sizeof( cl_sampler), &ocl_sampler);
    ocl_err |= clSetKernelArg( ocl_kernel, 3, sizeof( cl_int), &image_width);
    ocl_err |= clSetKernelArg( ocl_kernel, 4, sizeof( cl_int), &image_height);

    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;

        RELEASE_CL_OBJECT( ocl_sampler, clReleaseSampler);
        RELEASE_CL_OBJECT( ocl_image_dst, clReleaseMemObject);
        RELEASE_CL_OBJECT( ocl_image_src, clReleaseMemObject);
        RELEASE_CL_OBJECT( ocl_kernel, clReleaseKernel);
        RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
        RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
        RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

        return 1;
    }


        // Queue the kernel up for execution
    size_t local_work_size[2] = { 16, 16};
    size_t global_work_size[2] = { RoundUp( local_work_size[0], image_width), RoundUp( local_work_size[1], image_height) };

    ocl_err = clEnqueueNDRangeKernel(
        ocl_command_queue,
        ocl_kernel,
        2,
        nullptr,
        global_work_size, local_work_size, 0, nullptr, nullptr
    );
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error queuing kernel for execution." << ocl_err << std::endl;
        

        RELEASE_CL_OBJECT( ocl_sampler, clReleaseSampler);
        RELEASE_CL_OBJECT( ocl_image_dst, clReleaseMemObject);
        RELEASE_CL_OBJECT( ocl_image_src, clReleaseMemObject);
        RELEASE_CL_OBJECT( ocl_kernel, clReleaseKernel);
        RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
        RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
        RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

        return 1;
    }


    size_t origin[3] = { 0, 0, 0};
    size_t region[3] = { (size_t) image_width, (size_t) image_height, 1};

    if( b_use_map_buffer)
    {
        size_t row_pitch = 0;

        // Map the result back to a host buffer
        char *buffer = ( char *) clEnqueueMapImage(
            ocl_command_queue, ocl_image_dst, CL_TRUE,
            CL_MAP_READ, origin, region, &row_pitch,    // The Row pitch must be explicitly read back rather than assumed to be equal to width * bytesPerPixel
            nullptr, 0, nullptr, nullptr, &ocl_err
        );
        if( ocl_err != CL_SUCCESS)
        {
            std::cerr << "Error mapping result buffer." << std::endl;

            RELEASE_CL_OBJECT( ocl_sampler, clReleaseSampler);
            RELEASE_CL_OBJECT( ocl_image_dst, clReleaseMemObject);
            RELEASE_CL_OBJECT( ocl_image_src, clReleaseMemObject);
            RELEASE_CL_OBJECT( ocl_kernel, clReleaseKernel);
            RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
            RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
            RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

            return 1;
        }



        // Save the image out to disk
        if( !SaveImage( output_file_name.c_str(), buffer, image_width, image_height, row_pitch))
        {
            std::cerr << "Error writing output image: " << output_file_name << std::endl;

            RELEASE_CL_OBJECT( ocl_sampler, clReleaseSampler);
            RELEASE_CL_OBJECT( ocl_image_dst, clReleaseMemObject);
            RELEASE_CL_OBJECT( ocl_image_src, clReleaseMemObject);
            RELEASE_CL_OBJECT( ocl_kernel, clReleaseKernel);
            RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
            RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
            RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

            return 1;
        }



        // unmap image buffer
        ocl_err = clEnqueueUnmapMemObject( ocl_command_queue, ocl_image_dst, buffer, 0, nullptr, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            std::cerr << "clEnqueueUnmapMemObject() Failed." << std::endl;

            RELEASE_CL_OBJECT( ocl_sampler, clReleaseSampler);
            RELEASE_CL_OBJECT( ocl_image_dst, clReleaseMemObject);
            RELEASE_CL_OBJECT( ocl_image_src, clReleaseMemObject);
            RELEASE_CL_OBJECT( ocl_kernel, clReleaseKernel);
            RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
            RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
            RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

            return 1;
        }


    }
    else
    {
        size_t row_pitch = image_width * 4;

            // Read the output back to the host
        char *buffer = new char[ image_width * image_height * 4];    

        ocl_err = clEnqueueReadImage( ocl_command_queue, ocl_image_dst, CL_TRUE, origin, region, 0, 0, buffer, 0, nullptr, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            std::cerr << "Error reading result buffer." << std::endl;

            delete buffer;
            buffer = nullptr;

            RELEASE_CL_OBJECT( ocl_sampler, clReleaseSampler);
            RELEASE_CL_OBJECT( ocl_image_dst, clReleaseMemObject);
            RELEASE_CL_OBJECT( ocl_image_src, clReleaseMemObject);
            RELEASE_CL_OBJECT( ocl_kernel, clReleaseKernel);
            RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
            RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
            RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

            return 1;
        }



        // Save the image out to disk
        if( !SaveImage( output_file_name.c_str(), buffer, image_width, image_height, row_pitch))
        {
            std::cerr << "Error writing output image: " << output_file_name << std::endl;

            delete buffer;
            buffer = nullptr;

            RELEASE_CL_OBJECT( ocl_sampler, clReleaseSampler);
            RELEASE_CL_OBJECT( ocl_image_dst, clReleaseMemObject);
            RELEASE_CL_OBJECT( ocl_image_src, clReleaseMemObject);
            RELEASE_CL_OBJECT( ocl_kernel, clReleaseKernel);
            RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
            RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
            RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

            return 1;
        }

        delete buffer;
        buffer = nullptr;
    }


    RELEASE_CL_OBJECT( ocl_sampler, clReleaseSampler);
    RELEASE_CL_OBJECT( ocl_image_dst, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_image_src, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_kernel, clReleaseKernel);
    RELEASE_CL_OBJECT( ocl_program, clReleaseProgram);
    RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
    RELEASE_CL_OBJECT( ocl_context, clReleaseContext);

    return 0;
}


/**
 * @brief CreateContext() : returns OpenCL context.
 */
cl_context CreateContext( int platform_used)
{
    // variable declaration
    cl_int ocl_err;
    cl_uint ocl_num_platforms;
    cl_platform_id *ocl_platform_ids;
    cl_platform_id ocl_platform_id;
    cl_context ocl_context = nullptr;

    // code
    ocl_err = clGetPlatformIDs( 0, nullptr, &ocl_num_platforms);
    if( (ocl_err != CL_SUCCESS) || ( ocl_num_platforms <= 0))
    {
        std::cerr << "Failed to find any OpenCL platforms." << std::endl;
        return nullptr;
    }

        // First, select an OpenCL platform to run on.
    ocl_platform_ids = new cl_platform_id[ocl_num_platforms];
    ocl_err = clGetPlatformIDs( ocl_num_platforms, ocl_platform_ids, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        delete ocl_platform_ids;
        std::cerr << "Failed to find any OpenCL platforms." << std::endl;
        return nullptr;
    }

    if( (platform_used < 0) || ( platform_used >= ocl_num_platforms))
    {
        platform_used = DEFAULT_PLATFORM;
    }

        // Get selected platform
    ocl_platform_id = ocl_platform_ids[platform_used];

    DisplayPlatformInfo( ocl_platform_id, CL_PLATFORM_PROFILE, "CL_PLATFORM_PROFILE");
    DisplayPlatformInfo( ocl_platform_id, CL_PLATFORM_VERSION, "CL_PLATFORM_VERSION");
    DisplayPlatformInfo( ocl_platform_id, CL_PLATFORM_VENDOR, "CL_PLATFORM_VENDOR");

        // Next, create an OpenCL context on the platform. Attempt to create a GPU-based context, and
        // that fails, try to create a CPU-based context.
    cl_context_properties ocl_context_properties[] =
    {
        CL_CONTEXT_PLATFORM, (cl_context_properties) ocl_platform_id,
        0
    };

    ocl_context = clCreateContextFromType( ocl_context_properties, CL_DEVICE_TYPE_GPU, nullptr, nullptr, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cout << "Could not create GPU context, trying CPU..." << std::endl;

        ocl_context = clCreateContextFromType( ocl_context_properties, CL_DEVICE_TYPE_CPU, nullptr, nullptr, &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            delete ocl_platform_ids;
            std::cerr << "Failed to create an OpenCL GPU or CPU context." << std::endl;
            return nullptr;
        }
    }

    delete ocl_platform_ids;

    return ocl_context;
}


/**
 * @brief CreateCommandQueue() : create and return OpenCL command-queue
 */
cl_command_queue CreateCommandQueue( cl_context ocl_context, cl_device_id *ocl_device)
{
    // variable declarations
    cl_int ocl_err;
    cl_device_id *ocl_devices = nullptr;
    cl_command_queue ocl_command_queue = nullptr;
    size_t device_buffer_size = 0;

    // code
        // First get the size of the device buffer
    ocl_err = clGetContextInfo( ocl_context, CL_CONTEXT_DEVICES, 0, nullptr, &device_buffer_size);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Failed call to clGetContextInfo( ..., CL_CONTEXT_DEVICES, ...)" << std::endl;
        return nullptr;
    }

    if( device_buffer_size <= 0)
    {
        std::cerr << "No devices available.";
        return nullptr;
    }

        // Allocate memory for the devices buffer
    ocl_devices = new cl_device_id[ device_buffer_size / sizeof(cl_device_id)];
    ocl_err = clGetContextInfo( ocl_context, CL_CONTEXT_DEVICES, device_buffer_size, ocl_devices, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Failed to get device IDs." << std::endl;
        delete []ocl_devices;
        return nullptr;
    }

        // In this example, we just choose the first available device.
        // In a real program, you would likely use all available devices or choose the highest
        // performance device based on OpenCL device queries.
    ocl_command_queue = clCreateCommandQueue( ocl_context, ocl_devices[0], 0, nullptr);
    if( ocl_command_queue == nullptr)
    {
        std::cerr << "Failed to create OpenCL command-queue for device 0." << std::endl;
        delete []ocl_devices;
        return nullptr;
    }

    *ocl_device = ocl_devices[0];
    delete []ocl_devices;
    return ocl_command_queue;
}

/**
 * @brief CreateProgram() : Create OpenCL program from source file
 * 
 * @description: 
 *          A program object in OpenCL stores the compiled executable code for all of the devices
 *          that are attached to the context.
 */
cl_program CreateProgram( cl_context ocl_context, cl_device_id ocl_device, const char *file_name)
{
    // variable declaration
    cl_int ocl_err;
    cl_program ocl_program;

    // code
    std::ifstream kernel_file( file_name, std::ios::in);
    if( !kernel_file.is_open())
    {
        std::cerr << "Failed to open file for reading: " << file_name << std::endl;
        return nullptr;
    }

    std::ostringstream oss;
    oss << kernel_file.rdbuf();

    std::string src_std_str = oss.str();
    const char *src_str = src_std_str.c_str();

    ocl_program = clCreateProgramWithSource( ocl_context, 1, (const char **)&src_str, nullptr, nullptr);
    if( ocl_program == nullptr)
    {
        std::cerr << "Failed to create OpenCL program from source." << std::endl;
        return nullptr;
    }

    ocl_err = clBuildProgram( ocl_program, 0, nullptr, nullptr, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        // Determine the reason for the error
        size_t log_size = 0;
        clGetProgramBuildInfo( ocl_program, ocl_device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);

        if( log_size > 0)
        {
            char *build_log = new char[log_size + 1];
            
            clGetProgramBuildInfo( ocl_program, ocl_device, CL_PROGRAM_BUILD_LOG, sizeof( build_log), build_log, nullptr);
            std::cerr << "Error in Program: " << std::endl;
            std::cerr << build_log;

            delete build_log;
        }
        else
        {
            std::cerr << "Error in Program" << std::endl;
        }
    }

    return ocl_program;
}

/**
 * @brief RoundUp()
 */
size_t RoundUp( int group_size, int global_size)
{
    // code
    int r = global_size % group_size;
    if( r == 0)
    {
        return global_size;
    }
    else
    {
        return global_size + group_size - r;
    }
}

/**
 * @brief LoadImage()
 */
cl_mem LoadImage( cl_context ocl_context, const char *file_name, int &width, int &height)
{
    // code
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType( file_name, 0);
    FIBITMAP *image = FreeImage_Load( format, file_name);

        // convert to 32-bit image
    FIBITMAP *temp = image;
    image = FreeImage_ConvertTo32Bits( image);
    FreeImage_Unload( temp);

    width = FreeImage_GetWidth( image);
    height = FreeImage_GetHeight( image);

    // Create OpenCL image
    cl_image_format ocl_image_format;
    ocl_image_format.image_channel_order = CL_RGBA;
    ocl_image_format.image_channel_data_type = CL_UNORM_INT8;

    cl_int ocl_err;
    cl_mem ocl_image;
    
    ocl_image = clCreateImage2D(
        ocl_context,
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &ocl_image_format,
        width, height, 0,
        FreeImage_GetBits( image),
        &ocl_err
    );
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error creating CL image object." << std::endl;

        FreeImage_Unload( image);
        return nullptr;
    }

    return ocl_image;
}


/**
 * @brief SaveImage()
*/
bool SaveImage( const char *file_name, char *buffer, int image_width, int image_height, int row_pitch)
{
    // code
    FREE_IMAGE_FORMAT format = FreeImage_GetFIFFromFilename( file_name);
    FIBITMAP *image = FreeImage_ConvertFromRawBits(
        (BYTE*) buffer,
        image_width, image_height,
        row_pitch, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00);
    
    return FreeImage_Save( format, image, file_name);
}

