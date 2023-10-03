
#include "OGL.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "OpenCLUtil.h"

namespace OpenCLUtil
{
    static cl_context g_ocl_context = nullptr;
    static cl_command_queue g_ocl_command_queue = nullptr;
    static cl_device_id g_ocl_device_id = nullptr;

    /**
     * @brief IsInitialized()
     */
    bool IsInitialized()
    {
        // code
        return (g_ocl_context != nullptr) && (g_ocl_command_queue != nullptr);
    }

    /**
     * @brief CreateContext(): return OpenCL context if succeded.
     */
    static cl_context CreateContext( cl_device_type device_type)
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
            Log( "clGetPlatformIDs() Failed (%d).", ocl_err);
            return nullptr;
        }

        p_ocl_platform_ids = new cl_platform_id[ ocl_num_platforms];
        ocl_err = clGetPlatformIDs( ocl_num_platforms, p_ocl_platform_ids, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            Log( "clGetPlatformIDs() Failed (%d).", ocl_err);

            delete p_ocl_platform_ids;
            p_ocl_platform_ids = nullptr;

            return nullptr;
        }

        bool b_found = false;

        for( int i = 0; i < ocl_num_platforms; ++i)
        {
            ocl_platform_id = p_ocl_platform_ids[i];

            cl_uint ocl_device_count = 0;

            ocl_err = clGetDeviceIDs( ocl_platform_id, device_type, 0, nullptr, &ocl_device_count);
            if( ocl_device_count > 0)
            {
                b_found = true;
                break;
            }
        }

        delete p_ocl_platform_ids;
        p_ocl_platform_ids = nullptr;

        if( b_found == false)
        {
            Log( "No Platform Found.");
            return nullptr;
        }

        // create context on the platform.
        cl_context_properties ocl_context_properties[] =
        {
            CL_CONTEXT_PLATFORM, ( cl_context_properties) ocl_platform_id,
            CL_GL_CONTEXT_KHR, ( cl_context_properties) wglGetCurrentContext(),
            CL_WGL_HDC_KHR, ( cl_context_properties) wglGetCurrentDC(),
            0
        };

        ocl_context = clCreateContextFromType( ocl_context_properties, device_type, nullptr, nullptr, &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            Log( "Could not create Context.");
            return nullptr;
        }

        size_t param_length;
        ocl_err = clGetPlatformInfo( ocl_platform_id, CL_PLATFORM_VENDOR, 0, nullptr, &param_length);
        if( ocl_err != CL_SUCCESS)
        {
            Log( "clGetPlatformInfo() Failed (%d).", ocl_err);
        }

        char *str = new char[param_length + 1];
        ocl_err = clGetPlatformInfo( ocl_platform_id, CL_PLATFORM_VENDOR, param_length, str, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            Log( "clGetPlatformInfo() Failed (%d).", ocl_err);
        }
        else
        {
            Log( "OpenCL Platform Vender = %s", str);
        }

        delete str;

        return ocl_context;
    }

    /**
     * @brief CreateCommandQueue(): create and return OpenCL command-queue for first device
     */
    static cl_command_queue CreateCommandQueue( cl_context ocl_context, cl_device_id *out_ocl_device)
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
            Log( "clGetContextInfo() Failed ( %d).", ocl_err);
            return nullptr;
        }

        if( device_buffer_size <= 0)
        {
            Log( "No devices available.");
            return nullptr;
        }

            // Allocate memory for the devices
        p_ocl_devices = new cl_device_id[ device_buffer_size / sizeof( cl_device_id)];
        ocl_err = clGetContextInfo( ocl_context, CL_CONTEXT_DEVICES, device_buffer_size, p_ocl_devices, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            Log( "clGetContextInfo() Failed ( %d).", ocl_err);
            delete p_ocl_devices;
            p_ocl_devices = nullptr;
            return nullptr;
        }

            // get first device
        *out_ocl_device = p_ocl_devices[0];

        delete p_ocl_devices;
        p_ocl_devices = nullptr;

            // create command queue
        ocl_cmd_queue = clCreateCommandQueue( ocl_context, *out_ocl_device, 0, &ocl_err);
        if( (ocl_cmd_queue == nullptr) || (ocl_err != CL_SUCCESS))
        {
            Log( "clCreateCommandQueue() Failed ( %d).", ocl_err);
            return nullptr;
        }

        return ocl_cmd_queue;
    }


    /**
     * @brief Initialize()
     */
    bool Initialize()
    {
        // code
        g_ocl_context = CreateContext( CL_DEVICE_TYPE_GPU);
        if( g_ocl_context == nullptr)
        {
            Log("CreateContext() Failed.");
            Unintialize();
            return false;
        }

        g_ocl_command_queue = CreateCommandQueue( g_ocl_context, &g_ocl_device_id);
        if( g_ocl_command_queue == nullptr)
        {
            Log("CreateCommandQueue() Failed.");
            Unintialize();
            return false;
        }

        return true;
    }

    /**
     * @brief Unintialize()
     */
    void Unintialize()
    {
        // code
        CL_OBJECT_RELEASE( g_ocl_context, clReleaseContext);
        CL_OBJECT_RELEASE( g_ocl_command_queue, clReleaseCommandQueue);
    }

    /**
     * @brief CreateProgram() : Create OpenCL program from source file
     * 
     * @description: 
     *          A program object in OpenCL stores the compiled executable code for all of the devices
     *          that are attached to the context.
     */
    cl_program CreateProgram(const char *file_name)
    {
        // variable declaration
        cl_int ocl_err;
        cl_program ocl_program;

        // code
        std::ifstream kernel_file( file_name, std::ios::in);
        if( !kernel_file.is_open())
        {
            Log( "Failed to open file for reading: %s.", file_name);
            return nullptr;
        }

        std::ostringstream oss;
        oss << kernel_file.rdbuf();

        std::string src_std_str = oss.str();
        const char *src_str = src_std_str.c_str();

        ocl_program = clCreateProgramWithSource( g_ocl_context, 1, (const char **)&src_str, nullptr, nullptr);
        if( ocl_program == nullptr)
        {
            Log( "Failed to create OpenCL program from source.");
            return nullptr;
        }

        ocl_err = clBuildProgram( ocl_program, 0, nullptr, nullptr, nullptr, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            // Determine the reason for the error
            size_t log_size = 0;
            clGetProgramBuildInfo( ocl_program, g_ocl_device_id, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);

            if( log_size > 0)
            {
                char *build_log = new char[log_size + 1];
                
                clGetProgramBuildInfo( ocl_program, g_ocl_device_id, CL_PROGRAM_BUILD_LOG, log_size, build_log, nullptr);
                Log( "Error in Program: %s.", build_log);

                delete build_log;
            }
            else
            {
                Log( "Error in Program.");
            }

            return nullptr;
        }

        return ocl_program;
    }


    /**
     * @brief GetContext()
     */
    cl_context GetContext()
    {
        return g_ocl_context;
    }

    /**
     * @brief GetCommandQueue()
     */
    cl_command_queue GetCommandQueue()
    {
        return g_ocl_command_queue;
    }

}