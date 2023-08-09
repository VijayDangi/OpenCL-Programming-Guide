/**
 * @author : Vijaykumar Dangi
 * @date   : 1-August-2023
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <CL/cl.h>

#include "Info.hpp"

#define DEFAULT_PLATFORM 0
#define DEFAULT_USE_MAP 0
#define NUM_BUFFER_ELEMENTS 100

#define RELEASE_ELEMENT_AND_CLEAR_VECTOR( vec, release_func)  \
    for( int x = 0; x < vec.size(); ++x)    \
    {   \
        if( vec[x]) \
        {   \
            release_func( vec[x]);  \
            vec[x] = nullptr;   \
        }   \
    }   \
    vec.clear();



/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // function declaration
    cl_context CreateContext( int platform_used);
    cl_command_queue CreateCommandQueue( cl_context, cl_device_id* );
    cl_program CreateProgram( cl_context, cl_device_id, const char* );
    cl_program CreateProgramFromBinary( cl_context, cl_device_id, const char* );
    bool SaveProgramBinary( cl_program, cl_device_id, const char *);
    void Cleanup( cl_context, cl_command_queue, cl_program, cl_kernel);

    // variable declaration
    cl_context ocl_context = nullptr;
    cl_program ocl_program = nullptr;
    cl_device_id ocl_device = nullptr;
    cl_int ocl_err = 0;
    cl_uint num_platforms = 0;

    int platform_used = DEFAULT_PLATFORM;
    bool b_use_map_buffer = DEFAULT_USE_MAP;

    // code
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
        else
        {
            std::cout << "usage: %s --platform n --useMapBuffer" << std::endl;
            return 0;
        }
    }



        // Create an OpenCL context on first available platform
    ocl_context = CreateContext( platform_used);
    if( ocl_context == nullptr)
    {
        std::cerr << "Failed to create OpenCL Context." << std::endl;
        return 1;
    }


        // Running for first time, so create program from source, save the binary on disk for future use.
    ocl_program = CreateProgram( ocl_context, ocl_device, "Square.cl");
    if( ocl_program == nullptr)
    {
        std::cerr << "CreateProgram() Failed." << __LINE__ << std::endl;

        Cleanup( ocl_context, nullptr, ocl_program, nullptr);
        return 1;
    }


    /***************************************************************************************************/
    // get num devices
    cl_uint num_devices;
    ocl_err = clGetProgramInfo( ocl_program, CL_PROGRAM_NUM_DEVICES, sizeof( cl_uint), &num_devices, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clGetProgramInfo() Failed." << __LINE__ << std::endl;

        Cleanup( ocl_context, nullptr, ocl_program, nullptr);
        return 1;
    }

    cl_device_id *ocl_devices = new cl_device_id[ num_devices];
    ocl_err = clGetProgramInfo( ocl_program, CL_PROGRAM_DEVICES, sizeof( cl_device_id) * num_devices, ocl_devices, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clGetProgramInfo() Failed." << __LINE__ << std::endl;

        Cleanup( ocl_context, nullptr, ocl_program, nullptr);
        return 1;
    }

    std::cout << "Num Device : " << num_devices << std::endl;


        // Create memory objects that will be used as arguments to kernel.
        // First create host memory arrays that will be used to store the arguments to the kernel.
    int *inputOutput = new int[ NUM_BUFFER_ELEMENTS * num_devices];

    for( int i = 0; i < NUM_BUFFER_ELEMENTS * num_devices; ++i)
    {
        inputOutput[ i] = i;
    }



    std::vector<cl_command_queue> queue;
    std::vector<cl_event> events;
    std::vector<cl_mem> buffers;
    std::vector<cl_kernel> kernels;




        // create memory object
    cl_mem buffer = clCreateBuffer( ocl_context, CL_MEM_READ_WRITE, sizeof( cl_int) * NUM_BUFFER_ELEMENTS * num_devices, nullptr, nullptr);
    if( buffer == nullptr)
    {
        delete inputOutput;
        delete ocl_devices;

        Cleanup( ocl_context, nullptr, ocl_program, nullptr);
        std::cerr << "Error creating memory objects." << std::endl;
        return 1;
    }
    buffers.push_back( buffer);

        // now for all devices other than the first create a sub-buffer
    for( int i = 1; i < num_devices; ++i)
    {
        cl_buffer_region region = 
        {
            NUM_BUFFER_ELEMENTS * i * sizeof(int),
            NUM_BUFFER_ELEMENTS * sizeof(int)
        };

        cl_mem buffer = clCreateSubBuffer(
                buffers[0],
                CL_MEM_READ_WRITE,
                CL_BUFFER_CREATE_TYPE_REGION,
                &region,
                &ocl_err
        );

        if( buffer == nullptr)
        {
            delete inputOutput;
            delete ocl_devices;

            Cleanup( ocl_context, nullptr, ocl_program, nullptr);

            RELEASE_ELEMENT_AND_CLEAR_VECTOR( buffers, clReleaseMemObject);

            std::cerr << "Error creating memory objects." << std::endl;
            return 1;
        }
    }

    

        // Create a command-queue for each devices and kernel object
    for( int i = 0; i < num_devices; ++i)
    {
        InfoDevice<cl_device_type>::display( ocl_devices[i], CL_DEVICE_TYPE, __To_String(CL_DEVICE_TYPE));

        cl_command_queue ocl_command_queue = clCreateCommandQueue( ocl_context, ocl_devices[i], 0, &ocl_err);
        if( ocl_command_queue == nullptr)
        {
            std::cerr << "clCreateCommandQueue() Failed." << __LINE__ << std::endl;

            delete inputOutput;
            delete ocl_devices;

            RELEASE_ELEMENT_AND_CLEAR_VECTOR( buffers, clReleaseMemObject);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( queue, clReleaseCommandQueue);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( kernels, clReleaseKernel);

            Cleanup( ocl_context, ocl_command_queue, ocl_program, nullptr);

            return 1;
        }

        queue.push_back( ocl_command_queue);

        
            // Create OpenCL Kernel
        cl_kernel kernel = clCreateKernel( ocl_program, "square", &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            std::cerr << "clCreateKernel() Failed." << __LINE__ << std::endl;

            delete inputOutput;
            delete ocl_devices;

            RELEASE_ELEMENT_AND_CLEAR_VECTOR( buffers, clReleaseMemObject);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( queue, clReleaseCommandQueue);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( kernels, clReleaseKernel);

            Cleanup( ocl_context, ocl_command_queue, ocl_program, nullptr);

            return 1;
        }

        
            // Set the kernel arguments ( result, a, b)
        ocl_err = clSetKernelArg( kernel, 0 /* argument index */, sizeof( cl_mem), &buffers[i]);
        if( ocl_err != CL_SUCCESS)
        {
            std::cerr << "clSetKernelArg() Failed." << __LINE__ << std::endl;

            delete inputOutput;
            delete ocl_devices;

            RELEASE_ELEMENT_AND_CLEAR_VECTOR( buffers, clReleaseMemObject);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( queue, clReleaseCommandQueue);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( kernels, clReleaseKernel);

            Cleanup( ocl_context, ocl_command_queue, ocl_program, nullptr);

            return 1;
        }
    
        kernels.push_back( kernel);
        
    }


    if( b_use_map_buffer)
    {
        cl_int *map_ptr = (cl_int *) clEnqueueMapBuffer( queue[0], buffers[0], CL_TRUE, CL_MAP_WRITE, 0, sizeof( cl_int) * NUM_BUFFER_ELEMENTS * num_devices, 0, nullptr, nullptr, &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            std::cerr << "clEnqueueMapBuffer() Failed." << __LINE__ << std::endl;

            delete inputOutput;
            delete ocl_devices;

            RELEASE_ELEMENT_AND_CLEAR_VECTOR( buffers, clReleaseMemObject);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( queue, clReleaseCommandQueue);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( kernels, clReleaseKernel);

            Cleanup( ocl_context, nullptr, ocl_program, nullptr);

            return 1;
        }

        for( int i = 0; i < NUM_BUFFER_ELEMENTS * num_devices; ++i)
        {
            map_ptr[i] = inputOutput[i];
        }

        ocl_err = clEnqueueUnmapMemObject( queue[0], buffers[0], map_ptr, 0, nullptr, nullptr);
    }
    else
    {
        ocl_err = clEnqueueWriteBuffer( queue[0], buffers[0], CL_TRUE, 0, sizeof( int) * NUM_BUFFER_ELEMENTS * num_devices, (void *)inputOutput, 0, nullptr, nullptr);
    }




        //submit kernel enqueue to each queue
    for( int i = 0; i < queue.size(); ++i)
    {
        cl_event event;

        size_t gWI = NUM_BUFFER_ELEMENTS;

            // Queue the kernel up for execution across the array
        ocl_err = clEnqueueNDRangeKernel( queue[i], kernels[i], 1, nullptr, (const size_t*) &gWI, nullptr, 0, nullptr, &event);
        if( ocl_err != CL_SUCCESS)
        {
            std::cerr << "Error queuing kernel for execution." << std::endl;

            delete inputOutput;
            delete ocl_devices;

            RELEASE_ELEMENT_AND_CLEAR_VECTOR( buffers, clReleaseMemObject);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( queue, clReleaseCommandQueue);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( kernels, clReleaseKernel);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( events, clReleaseEvent);

            Cleanup( ocl_context, nullptr, ocl_program, nullptr);
            return 1;
        }

        events.push_back( event);
    }

    // wait for commands to complete
    clWaitForEvents( events.size(), events.data()); // Technically, don't need this as we are doing a blocking read with in-order queue.
    RELEASE_ELEMENT_AND_CLEAR_VECTOR( events, clReleaseEvent);

    /***************************************************************************************************/

    if( b_use_map_buffer)
    {
        cl_int *map_ptr = (cl_int*) clEnqueueMapBuffer( queue[0], buffers[0], CL_TRUE, CL_MAP_READ, 0, sizeof( cl_int) * NUM_BUFFER_ELEMENTS * num_devices, 0, nullptr, nullptr, &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            std::cerr << "clEnqueueMapBuffer()." << std::endl;

            delete inputOutput;
            delete ocl_devices;

            RELEASE_ELEMENT_AND_CLEAR_VECTOR( buffers, clReleaseMemObject);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( queue, clReleaseCommandQueue);
            RELEASE_ELEMENT_AND_CLEAR_VECTOR( kernels, clReleaseKernel);

            Cleanup( ocl_context, nullptr, ocl_program, nullptr);
            return 1;
        }

        for( int i = 0; i < NUM_BUFFER_ELEMENTS * num_devices; ++i)
        {
            inputOutput[i] = map_ptr[i];
        }

        ocl_err = clEnqueueUnmapMemObject( queue[0], buffers[0], map_ptr, 0, nullptr, nullptr);
    }
    else
    {
        clEnqueueReadBuffer( queue[0], buffers[0], CL_TRUE, 0, sizeof( int) * NUM_BUFFER_ELEMENTS * num_devices, (void *)inputOutput, 0, nullptr, nullptr);
    }

        // Output the result buffer
    for( int i = 0; i < NUM_BUFFER_ELEMENTS * num_devices; ++i)
    {
        std::cout << inputOutput[i] << "\n";
    }

    std::cout << "Executed program Successfully." << std::endl;

    delete inputOutput;
    delete ocl_devices;

    RELEASE_ELEMENT_AND_CLEAR_VECTOR( buffers, clReleaseMemObject);
    RELEASE_ELEMENT_AND_CLEAR_VECTOR( queue, clReleaseCommandQueue);
    RELEASE_ELEMENT_AND_CLEAR_VECTOR( kernels, clReleaseKernel);

    Cleanup( ocl_context, nullptr, ocl_program, nullptr);

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
 * @brief CreateProgramFromBinary() : Create OpenCL program from binary file
 * 
 * @description: 
 *          The last step of calling clBuildProgram() matt seem strange. the program is already in binary format format,
 *          so why does it need to be rebuilt? The answet stems from the fact that the program binary may or may not
 *          contain executable code. If it is an intermediate representation, than OpenCL will still need to compile it
 *          into final executable.
 */
cl_program CreateProgramFromBinary( cl_context ocl_context, cl_device_id ocl_device, const char *file_name)
{
    // code
    FILE *fp = fopen( file_name, "rb");
    if( fp == nullptr)
    {
        return nullptr;
    }

        // Determine the size of the binary
    size_t binary_size;
    fseek( fp, 0, SEEK_END);
    binary_size = ftell( fp);
    rewind( fp);

        // Load binary from disk
    unsigned char *program_binary = new unsigned char[ binary_size];
    fread( program_binary, 1, binary_size, fp);
    fclose( fp);

    cl_int ocl_err = 0;
    cl_program ocl_program;
    cl_int ocl_binary_status;

    ocl_program = clCreateProgramWithBinary( ocl_context, 1, &ocl_device, &binary_size, (const unsigned char**)&program_binary, &ocl_binary_status, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error loading program binary." << std::endl;
        delete [] program_binary;

        return nullptr;
    }

    if( ocl_binary_status != CL_SUCCESS)
    {
        std::cerr << "Invalid binary for device." << std::endl;
        delete [] program_binary;

        return nullptr;
    }

    ocl_err = clBuildProgram( ocl_program, 0, nullptr, nullptr, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
            // Determine the reason for the error
        char build_log[16384];
        clGetProgramBuildInfo( ocl_program, ocl_device, CL_PROGRAM_BUILD_LOG, sizeof( build_log), build_log, nullptr);

        std::cerr << "Error in Kernel: " << std::endl;
        std::cerr << build_log;

        clReleaseProgram( ocl_program);
        delete [] program_binary;

        return nullptr;
    }

    delete [] program_binary;

    return ocl_program;
}

/**
 * @brief SaveProgramBinary() : read program's binary and store in file for specied device id.
 * 
 * @description: Assume that program object was already created and build from source.
 */
bool SaveProgramBinary( cl_program ocl_program, cl_device_id ocl_device, const char *binary_file_name)
{
    // variable declaration
    cl_uint num_devices = 0;
    cl_int ocl_err;

    // code
        // 1. Query for number of devices attached to program
    ocl_err = clGetProgramInfo( ocl_program, CL_PROGRAM_NUM_DEVICES, sizeof( cl_uint), &num_devices, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error queryinh for number of devices." << std::endl;
        return false;
    }

        // 2. Get all device ids
    cl_device_id *ocl_devices = new cl_device_id[num_devices];
    ocl_err = clGetProgramInfo( ocl_program, CL_PROGRAM_DEVICES, sizeof( cl_device_id) * num_devices, ocl_devices, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error queryinh for devices." << std::endl;
        delete []ocl_devices;
        return false;
    }

        // 3. Determine the size of each prigram binary
    size_t *program_binary_sizes = new size_t[num_devices];
    ocl_err = clGetProgramInfo( ocl_program, CL_PROGRAM_BINARY_SIZES, sizeof( size_t) * num_devices, program_binary_sizes, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error queryinh for program binary sizes." << std::endl;
        delete []ocl_devices;
        delete []program_binary_sizes;
        return false;
    }

    unsigned char **program_binaries = new unsigned char*[ num_devices];
    for( cl_uint i = 0; i < num_devices; ++i)
    {
        program_binaries[i] = new unsigned char[program_binary_sizes[i]];
    }

        // 4. Get all of the program binaries
    ocl_err = clGetProgramInfo( ocl_program, CL_PROGRAM_BINARIES, sizeof( unsigned char *) * num_devices, program_binaries, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error queryinh for program binary." << std::endl;
        delete []ocl_devices;
        delete []program_binary_sizes;
        
        for( cl_uint i = 0; i < num_devices; ++i)
        {
            delete []program_binaries[i];
        }

        delete [] program_binaries;

        return false;
    }

        // 5. Finally store the binaries for the device requested out to disk for future reading.
    for( cl_uint i = 0; i < num_devices; ++i)
    {
            // Store the binary just for the device requested.
            // In a scenario where multiple devices were being used you would save all the binaries.
        if( ocl_devices[i] == ocl_device)
        {
            FILE *fp =  fopen( binary_file_name, "wb");
            fwrite( program_binaries[i], 1, program_binary_sizes[i], fp);
            fclose( fp);
            break;
        }
    }

    // cleanup

    delete []ocl_devices;
    delete []program_binary_sizes;
    
    for( cl_uint i = 0; i < num_devices; ++i)
    {
        delete []program_binaries[i];
    }

    delete [] program_binaries;

    return true;
}


/**
 * @brief Cleanup()
 */
void Cleanup( cl_context ocl_context, cl_command_queue ocl_command_queue, cl_program ocl_program, cl_kernel ocl_kernel)
{
    //code
    if( ocl_kernel)
    {
        clReleaseKernel( ocl_kernel);
        ocl_kernel = nullptr;
    }

    if( ocl_program)
    {
        clReleaseProgram( ocl_program);
        ocl_program = nullptr;
    }

    if( ocl_command_queue)
    {
        clReleaseCommandQueue( ocl_command_queue);
        ocl_command_queue = nullptr;
    }

    if( ocl_context)
    {
        clReleaseContext( ocl_context);
        ocl_context = nullptr;
    }
}

