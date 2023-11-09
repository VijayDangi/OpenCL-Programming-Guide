/**
 * @author : Vijaykumar Dangi
 * @date   : 
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <limits>

#include "OpenCLUtil.h"

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

cl_program ocl_matrix_multiplication_program;
cl_kernel ocl_matrix_multiplication_kernel;
cl_mem ocl_A_matrix;
cl_mem ocl_B_matrix;
cl_mem ocl_C_matrix;

const int M_DIMENSION = 1000;
const int N_DIMENSION = 1000;
const int P_DIMENSION = 1000;

float A[N_DIMENSION][P_DIMENSION];
float B[P_DIMENSION][M_DIMENSION];
float C[N_DIMENSION][M_DIMENSION];
float C_GPU[N_DIMENSION][M_DIMENSION];

/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // function declaration
    void matrix_multiplication_cpu( int m_dim, int n_dim, int p_dim,
        float *A, float *B, float *Out_C
    );
    float get_random_value();
    void  cleanup();

    // variable declaration
    cl_int ocl_err;


    // code
        /******** Initialize OpenCL ***********/
    ocl_context = CreateContext( 0, CL_DEVICE_TYPE_GPU);
    if( ocl_context == nullptr)
    {
        std::cerr << "CreateContext() Failed.";
        cleanup();
        return EXIT_FAILURE;
    }

    ocl_command_queue = CreateCommandQueue( ocl_context, CL_QUEUE_PROFILING_ENABLE, &ocl_device);
    if( ocl_command_queue == nullptr)
    {
        std::cerr << "CreateCommandQueue() Failed.";
        cleanup();
        return EXIT_FAILURE;
    }

        // Create OpenCL program and kernel
    ocl_matrix_multiplication_program = CreateProgram( ocl_context, ocl_device, "matrix_mult.cl");
    if( ocl_matrix_multiplication_program == nullptr)
    {
        std::cerr << "CreateProgram() Failed." << std::endl;
        cleanup();
        return EXIT_FAILURE;
    }

    ocl_matrix_multiplication_kernel = clCreateKernel( ocl_matrix_multiplication_program, "mat_mul", &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clCreateKernel() Failed." << std::endl;
        cleanup();
        return EXIT_FAILURE;
    }

    
        // Fill Matrices
    for( int i = 0; i < N_DIMENSION; ++i)
    {
        for( int j = 0; j < P_DIMENSION; ++j)
        {
            A[i][j] = get_random_value();
        }
    }

    for( int i = 0; i < P_DIMENSION; ++i)
    {
        for( int j = 0; j < M_DIMENSION; ++j)
        {
            B[i][j] = get_random_value();
        }
    }


        // Create OpenCL buffer
    ocl_A_matrix = clCreateBuffer( ocl_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, N_DIMENSION * P_DIMENSION * sizeof( float), &A[0][0], &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "(" << __LINE__ << ") clCreateBuffer() Failed." << std::endl;
        cleanup();
        return EXIT_FAILURE;
    }

    ocl_B_matrix = clCreateBuffer( ocl_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, P_DIMENSION * M_DIMENSION * sizeof( float), &B[0][0], &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "(" << __LINE__ << ") clCreateBuffer() Failed." << std::endl;
        cleanup();
        return EXIT_FAILURE;
    }

    ocl_C_matrix = clCreateBuffer( ocl_context, CL_MEM_READ_WRITE, N_DIMENSION * M_DIMENSION * sizeof( float), nullptr, &ocl_err);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "(" << __LINE__ << ") clCreateBuffer() Failed." << std::endl;
        cleanup();
        return EXIT_FAILURE;
    }

        //////////////// CPU
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    matrix_multiplication_cpu( M_DIMENSION, N_DIMENSION, P_DIMENSION, &A[0][0], &B[0][0], &C[0][0]);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double>  elapsed_seconds = end - start;
    printf( "CPU Time Required : %lf sec\n", elapsed_seconds.count());
    fflush(stdout);



        //////////////// GPU
    cl_event profile_event;

    size_t max_compute_device = 0;
    clGetDeviceInfo( ocl_device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof( size_t), &max_compute_device, nullptr);
    std::cout << max_compute_device << "\n";

    clSetKernelArg( ocl_matrix_multiplication_kernel, 0, sizeof( cl_int), &M_DIMENSION);
    clSetKernelArg( ocl_matrix_multiplication_kernel, 1, sizeof( cl_int), &N_DIMENSION);
    clSetKernelArg( ocl_matrix_multiplication_kernel, 2, sizeof( cl_int), &P_DIMENSION);
    clSetKernelArg( ocl_matrix_multiplication_kernel, 3, sizeof( cl_mem), &ocl_A_matrix);
    clSetKernelArg( ocl_matrix_multiplication_kernel, 4, sizeof( cl_mem), &ocl_B_matrix);
    clSetKernelArg( ocl_matrix_multiplication_kernel, 5, sizeof( cl_mem), &ocl_C_matrix);

    start = std::chrono::steady_clock::now();

    size_t global_work_size[] = { N_DIMENSION};
    size_t local_work_size[] = { N_DIMENSION / 25};

    ocl_err = clEnqueueNDRangeKernel(
        ocl_command_queue, ocl_matrix_multiplication_kernel, 1, nullptr,
        global_work_size, local_work_size, 0, nullptr, &profile_event
    );
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueNDRangeKernel() Failed." << std::endl;
        cleanup();
        return EXIT_FAILURE;
    }

    clFinish(ocl_command_queue);

    end = std::chrono::steady_clock::now();
    elapsed_seconds = end - start;
    printf( "GPU Time Required : %lf sec\n", elapsed_seconds.count());
    fflush( stdout);

    cl_ulong event_start_time = 0;
    cl_ulong event_end_time = 0;
    size_t run_time;

    ocl_err = clGetEventProfilingInfo( profile_event, CL_PROFILING_COMMAND_START, sizeof( cl_ulong), &event_start_time, nullptr);
    ocl_err = clGetEventProfilingInfo( profile_event, CL_PROFILING_COMMAND_END, sizeof( cl_ulong), &event_end_time, nullptr);

    run_time = event_end_time - event_start_time;
    printf( "GPU Time Required (Profiling): %lf sec\n", ((run_time / 1000.0f) / 1000.0f) / 1000.0f );

    clReleaseEvent( profile_event);

        // OUTPUT CHECKING
    ocl_err = clEnqueueReadBuffer( ocl_command_queue, ocl_C_matrix, CL_TRUE, 0, sizeof(float) * N_DIMENSION * M_DIMENSION, &C_GPU[0][0], 0, nullptr, nullptr);
    if( ocl_err != CL_SUCCESS)
    {
        std::cerr << "clEnqueueReadBuffer() Failed." << std::endl;
        cleanup();
        return EXIT_FAILURE;
    }

    int i, j;
    bool bFailed = false;
    float failedResult;

    for( i = 0; i < N_DIMENSION; ++i)
    {
        for( j = 0; j < M_DIMENSION; ++j)
        {
            if( fabs( C_GPU[i][j] - C[i][j]) > 0.000001f)
            {
                failedResult = C_GPU[i][j] - C[i][j];
                bFailed = true;
                break;
            }
        }
        if( bFailed)
        {
            break;
        }
    }


    if( bFailed)
    {
        std::cerr << "Computation Result Failed at " << i << ", " << j << " = " << C_GPU[i][j] << " - " << C[i][j] << " = " << failedResult << "\n";
    }

    cleanup();

    return 0;
}

/**
 * @brief get_random_value()
 * @return 
 */
float get_random_value()
{
    // code
    float val = ((float)rand() / (float)RAND_MAX);  // [0, 1]
    //val = std::numeric_limits<float>::min() + val * (std::numeric_limits<float>::max()*0.5f - std::numeric_limits<float>::min());
    
    val = val * 2.0f - 1.0f;
    val = val * 100000;

    return val;
}

/**
 * @brief matrix_multiplication_cpu()
 * @param m_dim 
 * @param n_dim 
 * @param p_dim 
 * @param A 
 * @param B 
 * @param Out_C 
 */
void matrix_multiplication_cpu(
    int m_dim, int n_dim, int p_dim,    // matrix dimensions
    float *A, float *B, float *Out_C    // A[n_dim][p_dim], B[p_dim][m_dim], Out_C[n_dim][m_dim]
)
{
    // variable declaration
    int i, j, k;

    // code
    for( i = 0; i < n_dim; ++i)
    {
        
        for( k = 0; k < p_dim; ++k)
        {
            for( j = 0; j < m_dim; ++j)
            {
                // Out_C[i][j] += A[i][k] * B[k][j];
                *(Out_C + ( i * n_dim + j)) += (*(A + (i * n_dim + k))) * (*(B + (k * p_dim + j)));
            }
        }
    }
}

/**
 * @brief cleanup()
 */
void  cleanup()
{
    // code
    RELEASE_CL_OBJECT( ocl_matrix_multiplication_program, clReleaseProgram);
    RELEASE_CL_OBJECT( ocl_matrix_multiplication_kernel, clReleaseKernel);
    RELEASE_CL_OBJECT( ocl_A_matrix, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_B_matrix, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_C_matrix, clReleaseMemObject);

    RELEASE_CL_OBJECT( ocl_context, clReleaseContext);
    RELEASE_CL_OBJECT( ocl_command_queue, clReleaseCommandQueue);
    RELEASE_CL_OBJECT( ocl_device, clReleaseDevice);
}
