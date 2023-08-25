
#include <iostream>
#include <fstream>
#include <sstream>
#include "OpenCLUtil.h"

/**
 * @brief CreateContext(): return OpenCL context if succeded.
 */
cl_context CreateContext( int platform_used)
{
    // variable declaration
    cl_int ocl_err;
    cl_uint ocl_num_platforms = 0;
    cl_platform_id *p_ocl_platform_ids = nullptr;
    cl_platform_id ocl_platform_id = nullptr;
    cl_context ocl_context = nullptr;

    // code
    ocl_err = clGetPlatformIDs( 0, nullptr, &ocl_num_platforms);
    if( (ocl_err != CL_SUCCESS) || ( ocl_num_platforms <= 0))
    {
        std::cerr << "clGetPlatformIDs() Failed (" << ocl_err << ")." << std::endl;
        return nullptr;
    }

    p_ocl_platform_ids = new cl_platform_id[ ocl_num_platforms];
    ocl_err = clGetPlatformIDs( ocl_num_platforms, p_ocl_platform_ids, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clGetPlatformIDs() Failed (" << ocl_err << ")." << std::endl;

        delete p_ocl_platform_ids;
        p_ocl_platform_ids = nullptr;

        return nullptr;
    }

    if( (platform_used < 0) || (platform_used >= ocl_num_platforms))
    {
        platform_used = 0;
    }

    ocl_platform_id = p_ocl_platform_ids[0];
    delete p_ocl_platform_ids;
    p_ocl_platform_ids = nullptr;

    // create context on the platform.
    cl_context_properties ocl_context_properties[] =
    {
        CL_CONTEXT_PLATFORM, ( cl_context_properties) ocl_platform_id,
        0
    };

    ocl_context = clCreateContextFromType( ocl_context_properties, CL_DEVICE_TYPE_GPU, nullptr, nullptr, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Could not create GPU Context, trying for CPU...\n";

        ocl_context = clCreateContextFromType( ocl_context_properties, CL_DEVICE_TYPE_CPU, nullptr, nullptr, &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            std::cerr << "Failed to create an OpenCL GPU and CPU context\n";
            return nullptr;
        }
    }

    return ocl_context;
}

/**
 * @brief CreateCommandQueue(): create and return OpenCL command-queue for first device
 */
cl_command_queue CreateCommandQueue( cl_context ocl_context, cl_device_id *out_ocl_device)
{
    // variable declaration
    cl_int ocl_err;
    cl_device_id *p_ocl_devices = nullptr;
    cl_command_queue ocl_cmd_queue = nullptr;
    size_t device_buffer_size = 0;

    // code
    ocl_err = clGetContextInfo( ocl_context, CL_CONTEXT_DEVICES, 0, nullptr, &device_buffer_size);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clGetContextInfo() Failed ( " << ocl_err << ").\n";
        return nullptr;
    }

    if( device_buffer_size <= 0)
    {
        std::cerr << "No devices available.\n";
        return nullptr;
    }

        // Allocate memory for the devices
    p_ocl_devices = new cl_device_id[ device_buffer_size / sizeof( cl_device_id)];
    ocl_err = clGetContextInfo( ocl_context, CL_CONTEXT_DEVICES, device_buffer_size, p_ocl_devices, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clGetContextInfo() Failed (" << ocl_err << ").\n";
        delete p_ocl_devices;
        p_ocl_devices = nullptr;
        return nullptr;
    }

        // get first device
    *out_ocl_device = p_ocl_devices[0];

    delete p_ocl_devices;
    p_ocl_devices = nullptr;

        // create command queue
    ocl_cmd_queue = clCreateCommandQueue( ocl_context, *out_ocl_device, 0, nullptr);
    if( ocl_cmd_queue == nullptr)
    {
        std::cerr << "clCreateCommandQueue() Failed (" << ocl_err << ").\n";
        return nullptr;
    }

    return ocl_cmd_queue;
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
            
            clGetProgramBuildInfo( ocl_program, ocl_device, CL_PROGRAM_BUILD_LOG, log_size, build_log, nullptr);
            std::cerr << "Error in Program: " << std::endl;
            std::cerr << build_log;

            delete build_log;
        }
        else
        {
            std::cerr << "Error in Program" << std::endl;
        }

        return nullptr;
    }

    return ocl_program;
}