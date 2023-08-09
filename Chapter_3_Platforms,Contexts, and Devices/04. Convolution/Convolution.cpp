/**
 * @author : Vijaykumar Dangi
 * @date   : 19-July-2023
 */

#include <iostream>
#include <fstream>
#include <sstream>

#ifdef __APPLE__
    #include <OpenCL/cl.h>
#else
    #include <CL/cl.h>
#endif

// constants
    // input signal
const unsigned int input_signal_width  = 8;
const unsigned int input_signal_height = 8;

cl_uint input_signal[ input_signal_width][input_signal_height] =
{
    { 3, 1, 1, 4, 8, 2, 1, 3},
    { 4, 2, 1, 1, 2, 1, 2, 3},
    { 4, 4, 4, 4, 3, 2, 2, 2},
    { 9, 8, 3, 8, 9, 0, 0, 0},
    { 9, 3, 3, 9, 0, 0, 0, 0},
    { 0, 9, 0, 8, 0, 0, 0, 0},
    { 3, 0, 8, 8, 9, 4, 4, 4},
    { 5, 9, 8, 1, 8, 1, 1, 1}
};

    // mask
const unsigned int mask_width  = 3;
const unsigned int mask_height = 3;

cl_uint mask[ mask_width][ mask_height] =
{
    { 1, 1, 1},
    { 1, 0, 1},
    { 1, 1, 1}
};

    // output signal
const unsigned int output_signal_width  = 6;
const unsigned int output_signal_height = 6;

cl_uint output_signal[ output_signal_width][output_signal_height];

/**
 * @brief checkErr()
 */
inline void checkErr( cl_int err, const char *name)
{
    // code
    if( err != CL_SUCCESS)
    {
        std::cerr << "Error: " << name << " ( " << err << ")" << std::endl;
        exit( EXIT_FAILURE);
    }
}

/**
 * @brief contextCallback() 
 */
void CL_CALLBACK contextCallback( const char *err_info, const void *private_info, size_t cb, void *user_data)
{
    // code
    std::cout << "Error occured during context use: " << err_info << std::endl;
    exit( EXIT_FAILURE);
}

/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // variable declaration
    cl_int ocl_err;
    cl_uint ocl_num_platforms = 0;
    cl_uint ocl_num_devices = 0;

    cl_platform_id *ocl_plaform_ids = nullptr;
    cl_device_id *ocl_device_ids = nullptr;

    cl_context ocl_context = nullptr;
    cl_command_queue ocl_queue = nullptr;

    cl_program ocl_program = nullptr;
    cl_kernel ocl_kernel = nullptr;

    cl_mem ocl_input_signal_buffer = nullptr;
    cl_mem ocl_output_signal_buffer = nullptr;
    cl_mem ocl_mask_buffer = nullptr;

    // code
        // get platform
    ocl_err = clGetPlatformIDs( 0, nullptr, &ocl_num_platforms);
    checkErr( (ocl_err != CL_SUCCESS) ? ocl_err : ( ocl_num_platforms <= 0 ? -1 : CL_SUCCESS), "clGetPlatformIDs()");

    ocl_plaform_ids = ( cl_platform_id *) alloca( sizeof( cl_platform_id) * ocl_num_platforms);
    ocl_err = clGetPlatformIDs( ocl_num_platforms, ocl_plaform_ids, nullptr);
    checkErr( ocl_err, "clGetPlatformIDs()");

        // get device
    cl_uint i;

    for( i = 0; i < ocl_num_platforms; ++i)
    {
        ocl_err = clGetDeviceIDs( ocl_plaform_ids[i], CL_DEVICE_TYPE_GPU, 0, nullptr, &ocl_num_devices);
        if( (ocl_err != CL_SUCCESS) && ( ocl_err != CL_DEVICE_NOT_FOUND))
        {
            checkErr( ocl_err, "clGetDeviceIDs()");
        }
        else if( ocl_num_devices > 0)
        {
            ocl_device_ids = ( cl_device_id *) alloca( sizeof( cl_device_id) * ocl_num_devices);

            ocl_err = clGetDeviceIDs( ocl_plaform_ids[i], CL_DEVICE_TYPE_GPU, ocl_num_devices, &ocl_device_ids[0], nullptr);
            checkErr( ocl_err, "clGetDeviceIDs()");
            break;
        }
    }

    if( ocl_device_ids == nullptr)
    {
        std::cout << "No CPU device found." << std::endl;
        exit( -1);
    }

        // create context, program, kernel
    cl_context_properties ocl_context_properties[] =
    {
        CL_CONTEXT_PLATFORM, (cl_context_properties)ocl_plaform_ids[i],
        0
    };

    ocl_context = clCreateContext( ocl_context_properties, ocl_num_devices, ocl_device_ids, &contextCallback, nullptr, &ocl_err);
    checkErr( ocl_err, "clCreateContext()");

    std::ifstream src_file("Convolution.cl");
    checkErr( src_file.is_open() ? CL_SUCCESS : -1, "reading Convolution.cl");

    std::string src_program_source( std::istreambuf_iterator<char>( src_file), (std::istreambuf_iterator<char>()));

    const char *src = src_program_source.c_str();
    size_t length = src_program_source.length();

    ocl_program = clCreateProgramWithSource( ocl_context, 1, &src, &length, &ocl_err);
    checkErr( ocl_err, "clCreateProgramWithSource");

    ocl_err = clBuildProgram( ocl_program, ocl_num_devices, ocl_device_ids, nullptr, nullptr, nullptr);
    checkErr( ocl_err, "clBuildProgram");;

    ocl_kernel = clCreateKernel( ocl_program, "convolve", &ocl_err);
    checkErr( ocl_err, "clCreateKernel");;


        // create memory object
    ocl_input_signal_buffer = clCreateBuffer(
                                ocl_context,
                                CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                sizeof( cl_uint) * input_signal_height * input_signal_width,
                                static_cast< void *>(input_signal), &ocl_err
                            );
    checkErr( ocl_err, "clCreateBuffer(input_signal)");

    ocl_mask_buffer = clCreateBuffer(
                        ocl_context,
                        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                        sizeof( cl_uint) * mask_width * mask_height,
                        static_cast< void *>(mask), &ocl_err
                    );
    checkErr( ocl_err, "clCreateBuffer(mask)");

    ocl_output_signal_buffer = clCreateBuffer(
                                ocl_context,
                                CL_MEM_WRITE_ONLY,
                                sizeof( cl_uint) * output_signal_width * output_signal_height,
                                nullptr, &ocl_err
                            );
    checkErr( ocl_err, "clCreateBuffer(output_signal)");

        // create command queue
    ocl_queue = clCreateCommandQueue( ocl_context, ocl_device_ids[0], 0, &ocl_err);
    checkErr( ocl_err, "clCreateCommandQueue");

        // Set kernel parameter
    ocl_err = clSetKernelArg( ocl_kernel, 0, sizeof( cl_mem), &ocl_input_signal_buffer);
    ocl_err |= clSetKernelArg( ocl_kernel, 1, sizeof( cl_mem), &ocl_mask_buffer);
    ocl_err |= clSetKernelArg( ocl_kernel, 2, sizeof( cl_mem), &ocl_output_signal_buffer);
    ocl_err |= clSetKernelArg( ocl_kernel, 3, sizeof( cl_uint), &input_signal_width);
    ocl_err |= clSetKernelArg( ocl_kernel, 4, sizeof( cl_uint), &mask_width);
    checkErr( ocl_err, "clSetKernelArg");

        // enqueue kernel
    const size_t global_work_size[1] = { output_signal_width * output_signal_height};
    const size_t local_work_size[1] = { 1};

    ocl_err = clEnqueueNDRangeKernel( ocl_queue, ocl_kernel, 1, nullptr, global_work_size, local_work_size, 0, nullptr, nullptr);
    checkErr( ocl_err, "clEnqueueNDRangeKernel");

    ocl_err = clEnqueueReadBuffer( ocl_queue, ocl_output_signal_buffer, CL_TRUE, 0, sizeof( cl_uint) * output_signal_width * output_signal_height, output_signal, 0, nullptr, nullptr);
    checkErr( ocl_err, "clEnqueueReadBuffer");

    for( int y = 0; y < output_signal_height; ++y)
    {
        for( int x = 0; x < output_signal_width; ++x)
        {
            std::cout << output_signal[x][y] << " ";
        }
        std::cout << std::endl;
    }


    //cleanup
    if( ocl_output_signal_buffer)
    {
        clReleaseMemObject( ocl_output_signal_buffer);
        ocl_output_signal_buffer = nullptr;
    }

    if( ocl_mask_buffer)
    {
        clReleaseMemObject( ocl_mask_buffer);
        ocl_mask_buffer = nullptr;
    }

    if( ocl_input_signal_buffer)
    {
        clReleaseMemObject( ocl_input_signal_buffer);
        ocl_input_signal_buffer = nullptr;
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

    if( ocl_queue)
    {
        clReleaseCommandQueue( ocl_queue);
        ocl_queue = nullptr;
    }

    if( ocl_context)
    {
        clReleaseContext( ocl_context);
        ocl_context = nullptr;
    }

    return 0;
}
