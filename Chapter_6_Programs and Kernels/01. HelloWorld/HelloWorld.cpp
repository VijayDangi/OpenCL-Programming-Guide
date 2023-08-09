/**
 * @author : Vijaykumar Dangi
 * @date   : 31-July-2023
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include <CL/cl.h>

const int ARRAY_SIZE = 1000;

/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // function declaration
    cl_context CreateContext();
    cl_command_queue CreateCommandQueue( cl_context, cl_device_id* );
    cl_program CreateProgram( cl_context, cl_device_id, const char* );
    cl_program CreateProgramFromBinary( cl_context, cl_device_id, const char* );
    bool SaveProgramBinary( cl_program, cl_device_id, const char *);
    bool CreateMemObjects( cl_context, cl_mem[3], float *, float *);
    void Cleanup( cl_context, cl_command_queue, cl_program, cl_kernel, cl_mem[3]);

    // variable declaration
    cl_context ocl_context = 0;
    cl_command_queue ocl_command_queue = 0;
    cl_program ocl_program = 0;
    cl_device_id ocl_device = 0;
    cl_kernel ocl_kernel = 0;
    cl_mem ocl_memory_objects[3] = { 0, 0, 0};
    cl_int ocl_err;

    // code
        // Create an OpenCL context on first available platform
    ocl_context = CreateContext();
    if( ocl_context == nullptr)
    {
        std::cerr << "Failed to create OpenCL Context." << std::endl;
        return 1;
    }

        // Create a command-queue on the first device available on the created context
    ocl_command_queue = CreateCommandQueue( ocl_context, &ocl_device);
    if( ocl_command_queue == nullptr)
    {
        Cleanup( ocl_context, ocl_command_queue, ocl_program, ocl_kernel, ocl_memory_objects);
        return 1;
    }

        // Create OpenCL program from HelloWorld.cl.bin kernel binary source
    ocl_program = CreateProgramFromBinary( ocl_context, ocl_device, "HelloWorld.cl.bin");
    if( ocl_program == nullptr)
    {
            // Running for first time, so create program from source, save the binary on disk for future use.
        ocl_program = CreateProgram( ocl_context, ocl_device, "HelloWorld.cl");
        if( ocl_program == nullptr)
        {
            Cleanup( ocl_context, ocl_command_queue, ocl_program, ocl_kernel, ocl_memory_objects);
            return 1;
        }

            // Save program on disk
        if( SaveProgramBinary( ocl_program, ocl_device, "HelloWorld.cl.bin") == false)
        {
            std::cerr << "Failed to write program binary." << std::endl;
            Cleanup( ocl_context, ocl_command_queue, ocl_program, ocl_kernel, ocl_memory_objects);
            return 1;
        }
    }
    else
    {
        std::cout << "Read program from binary." << std::endl;
    }

        // Create OpenCL Kernel
    ocl_kernel = clCreateKernel( ocl_program, "hello_kernel", nullptr);
    if( ocl_kernel == nullptr)
    {
        std::cerr << "Failed to create kernel" << std::endl;
        Cleanup( ocl_context, ocl_command_queue, ocl_program, ocl_kernel, ocl_memory_objects);
        return 1;
    }

        // Create memory objects that will be used as arguments to kernel.
        // First create host memory arrays that will be used to store the arguments to the kernel.
    float result[ ARRAY_SIZE];
    float a[ARRAY_SIZE];
    float b[ARRAY_SIZE];

    for( int i = 0; i < ARRAY_SIZE; ++i)
    {
        a[i] = i;
        b[i] = i * 2;
    }

    if( !CreateMemObjects( ocl_context, ocl_memory_objects, a, b))
    {
        Cleanup( ocl_context, ocl_command_queue, ocl_program, ocl_kernel, ocl_memory_objects);
        return 1;
    }

        // Set the kernel arguments ( result, a, b)
    ocl_err = clSetKernelArg( ocl_kernel, 0 /* argument index */, sizeof( cl_mem), &ocl_memory_objects[0]);
    ocl_err |= clSetKernelArg( ocl_kernel, 1, sizeof( cl_mem), &ocl_memory_objects[1]);
    ocl_err |= clSetKernelArg( ocl_kernel, 2, sizeof( cl_mem), &ocl_memory_objects[2]);

    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        Cleanup( ocl_context, ocl_command_queue, ocl_program, ocl_kernel, ocl_memory_objects);
        return 1;
    }


    size_t globalWorkSize[1] = { ARRAY_SIZE};
    size_t localWorkSize[1] = { 1 };

        // Queue the kernel up for execution across the array
    ocl_err = clEnqueueNDRangeKernel( ocl_command_queue, ocl_kernel, 1, nullptr, globalWorkSize, localWorkSize, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error queuing kernel for execution." << std::endl;
        Cleanup( ocl_context, ocl_command_queue, ocl_program, ocl_kernel, ocl_memory_objects);
        return 1;
    }


        // Read the output buffer back to the host
            // The third argument is a boolead "blocking_read" that determines whether the call should wait until the result are ready before returning.
            // In this example it is set to CL_TRUE, which means that it will not return until the kernel read is done.
            // It is guaranteed that operations that are put into the command-queue are executed in order ( unless the command-queue is created with CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE,
            // which is not done on this example).
            // The read will not occur until execution of the kernel is finished, and the read will not return until it is able to read the results back from the device.
    ocl_err = clEnqueueReadBuffer( ocl_command_queue, ocl_memory_objects[2], CL_TRUE, 0, ARRAY_SIZE * sizeof( float), result, 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "Error reading result buffer." << std::endl;
        Cleanup( ocl_context, ocl_command_queue, ocl_program, ocl_kernel, ocl_memory_objects);
        return 1;
    }

        // Output the result buffer
    for( int i = 0; i < ARRAY_SIZE; ++i)
    {
        std::cout << a[i] << " + " << b[i] << " = " << result[i] << "\n";
    }

    std::cout << "Executed program Successfully." << std::endl;

    Cleanup( ocl_context, ocl_command_queue, ocl_program, ocl_kernel, ocl_memory_objects);

    return 0;
}


/**
 * @brief CreateContext() : returns OpenCL context.
 */
cl_context CreateContext()
{
    // variable declaration
    cl_int ocl_err;
    cl_uint ocl_num_platforms;
    cl_platform_id ocl_first_platform_id;
    cl_context ocl_context = nullptr;

    // code
        // First, select an OpenCL platform to run on.
        // For this example, we simply choose the first availble platform.
        // Normally, you would query for all availble platforms and select the most appropriate one.
    ocl_err = clGetPlatformIDs( 1, &ocl_first_platform_id, &ocl_num_platforms);
    if( (ocl_err != CL_SUCCESS) || ( ocl_num_platforms <= 0))
    {
        std::cerr << "Failed to find any OpenCL platforms." << std::endl;
        return nullptr;
    }

        // Next, create an OpenCL context on the platform. Attempt to create a GPU-based context, and
        // that fails, try to create a CPU-based context.
    cl_context_properties ocl_context_properties[] =
    {
        CL_CONTEXT_PLATFORM, (cl_context_properties) ocl_first_platform_id,
        0
    };

    ocl_context = clCreateContextFromType( ocl_context_properties, CL_DEVICE_TYPE_GPU, nullptr, nullptr, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cout << "Could not create GPU context, trying CPU..." << std::endl;

        ocl_context = clCreateContextFromType( ocl_context_properties, CL_DEVICE_TYPE_CPU, nullptr, nullptr, &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            std::cerr << "Failed to create an OpenCL GPU or CPU context." << std::endl;
            return nullptr;
        }
    }

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
 * @brief CreateMemObjects() : Create OpenCL buffer and copt data into it
 */
bool CreateMemObjects( cl_context ocl_context, cl_mem ocl_mem_objects[3], float *a, float *b)
{
    // code
    ocl_mem_objects[0] = clCreateBuffer(    // create memory object which allocated in device memory and can be accessed directly by kernel function.
        ocl_context,
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,    // memory is read only and copied from host memory to device memory
        sizeof( float) * ARRAY_SIZE, a,
        nullptr
    );

    ocl_mem_objects[1] = clCreateBuffer( ocl_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof( float) * ARRAY_SIZE, b, nullptr);

    ocl_mem_objects[2] = clCreateBuffer(
        ocl_context,
        CL_MEM_READ_WRITE,  // memory is read and write
        sizeof( float) * ARRAY_SIZE, nullptr, nullptr
    );

    if(
        (ocl_mem_objects[0] == nullptr) ||
        (ocl_mem_objects[1] == nullptr) ||
        (ocl_mem_objects[2] == nullptr)
    )
    {
        std::cerr << "Error creating memory objects." << std::endl;
        return false;
    }

    return true;
}


/**
 * @brief Cleanup()
 */
void Cleanup( cl_context ocl_context, cl_command_queue ocl_command_queue, cl_program ocl_program, cl_kernel ocl_kernel, cl_mem ocl_mem_objects[3])
{
    //code
    if( ocl_mem_objects[0])
    {
        clReleaseMemObject( ocl_mem_objects[0]);
        ocl_mem_objects[0] = nullptr;
    }

    if( ocl_mem_objects[1])
    {
        clReleaseMemObject( ocl_mem_objects[1]);
        ocl_mem_objects[1] = nullptr;
    }

    if( ocl_mem_objects[2])
    {
        clReleaseMemObject( ocl_mem_objects[2]);
        ocl_mem_objects[2] = nullptr;
    }

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

