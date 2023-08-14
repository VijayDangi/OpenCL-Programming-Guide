/**
 * @author : Vijaykumar Dangi
 * @date : 15-Aug-2023
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>

    // enable OpenCL C++ Exception Wrapper
#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>

#define BUFFER_SIZE 20

int A[ BUFFER_SIZE];
int B[ BUFFER_SIZE];
int C[ BUFFER_SIZE];

static char kernel_source_code[] = 
    "__kernel void vec_add( __global int *a, __global int *b, __global int *c)      \n"
    "{                                                                              \n"
    "   size_t i = get_global_id(0);                                                \n"
    "                                                                               \n"
    "   c[i] = a[i] + b[i];                                                         \n"
    "}                                                                              \n"
    "                                                                               \n";

/**
 * @brief main() : Entry-point function
 */
int main( int argc, char **argv)
{
    // code
    cl_int ocl_err;

        // initialize arrays
    for( int i = 0; i < BUFFER_SIZE; ++i)
    {
        A[i] = i;
        B[i] = i * 2;
        C[i] = 0;
    }

    try
    {

            // get OpenCL platform
        std::vector<cl::Platform> platform_lists;
        cl::Platform::get( &platform_lists);

        // create OpenCL context using first platform
        cl_context_properties context_properties[] =
        {
            CL_CONTEXT_PLATFORM, (cl_context_properties)(platform_lists[0]()),
            0
        };

        cl::Context ocl_context( CL_DEVICE_TYPE_GPU, context_properties);

            // Query the set of devices attached to the context
        std::vector< cl::Device> ocl_devices = ocl_context.getInfo<CL_CONTEXT_DEVICES>();

            // Create Command queue
        cl::CommandQueue queue( ocl_context, ocl_devices[0], 0);

            // create program
        cl::Program::Sources sources( 1, std::make_pair( kernel_source_code, 0));
        cl::Program ocl_program( ocl_context, sources);
        // cl::Program ocl_program( ocl_context, program_source);

            // build program
        ocl_program.build(ocl_devices);

            // create buffer from A and copy host contents
        cl::Buffer aBuffer = cl::Buffer(
            ocl_context,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            BUFFER_SIZE * sizeof( cl_int),
            ( void *) &A[0]
        );

            // create buffer from B and copy host contents
        cl::Buffer bBuffer = cl::Buffer(
            ocl_context,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            BUFFER_SIZE * sizeof( cl_int),
            ( void *) &B[0]
        );

            // create output buffer
        cl::Buffer cBuffer = cl::Buffer(
            ocl_context,
            CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
            BUFFER_SIZE * sizeof( cl_int),
            ( void *) &C[0]
        );

            // Create OpenCL kernel
        cl::Kernel kernel( ocl_program, "vec_add");

            // Set kernel args
        kernel.setArg( 0, aBuffer);
        kernel.setArg( 1, bBuffer);
        kernel.setArg( 2, cBuffer);

            // Do the work
        queue.enqueueNDRangeKernel( kernel, cl::NullRange, cl::NDRange( BUFFER_SIZE), cl::NullRange);

            // map cbuffer to host pointer.
            // This enforces a sync with the host backing space
        int *output = ( int *) queue.enqueueMapBuffer( cBuffer, CL_TRUE, CL_MAP_READ, 0, BUFFER_SIZE * sizeof( int));

        for( int i = 0; i < BUFFER_SIZE; ++i)
        {
            std::cout << A[i] << " + " << B[i] << " = " << output[i] << " / " << C[i] << "\n";
        }
        std::cout << std::endl;

            // Finnalt realease our hold on accessing the memory
        ocl_err = queue.enqueueUnmapMemObject( cBuffer, (void *) output);

            // There is no need to perform on the final unmap or release any objects as this all
            // happens implicitly with the C++ Wrapper API.
    }
    catch( cl::Error err)
    {
        std::cerr << "Error: " << err.what() << "( " << err.err() << ")." << std::endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

