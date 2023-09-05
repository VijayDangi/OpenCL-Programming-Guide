/**
 * @author : Vijaykumar Dangi
 * @date   : 30-Aug-2023
 * 
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include "OpenCLUtil.h"

#include "../Common/FreeImage/x64/FreeImage.h"

#define To_String(x) #x

#define RELEASE_CL_OBJECT( obj, release_func) \
    if(obj) \
    {   \
        release_func(obj);    \
        obj = nullptr;  \
    }

#define CL_CHECK_ERROR(x, value) \
        if( x != value) \
        {   \
            std::cerr << "Error code (" << x <<") at line no. " << __LINE__ << " in function \"" << __FUNCTION__ << "\".\n";  \
            cleanup();  \
            exit( EXIT_FAILURE);    \
        }

#define NUM_ASYNCHRONOUS_ITERATIONS 10

typedef struct GraphData
{
    // (V) This contains a pointer to the edge list for each vertex.
    int *p_vertex_array = nullptr;

    // Vertex count
    int vertex_count = 0;

    // (E) This contains pointers to the vertices that each edge is attached to
    int *p_edge_array = nullptr;

    // Edge count
    int edge_count = 0;

    // (W) Weight array
    float *p_weight_array = nullptr;


    void release()
    {
        vertex_count = 0;
        edge_count = 0;

        if( p_vertex_array)
        {
            free( p_vertex_array);
            p_vertex_array = nullptr;
        }

        if( p_edge_array)
        {
            free( p_edge_array);
            p_edge_array = nullptr;
        }

        if( p_weight_array)
        {
            free( p_weight_array);
            p_weight_array = nullptr;
        }
    }

} GraphData;


// function declaration
void cleanup();

// variable declaration
cl_context ocl_context = nullptr;
cl_command_queue ocl_command_queue = nullptr;
cl_device_id ocl_device = nullptr;
cl_program ocl_program = nullptr;

cl_mem ocl_vertex_array = nullptr;
cl_mem ocl_edge_array = nullptr;
cl_mem ocl_weight_array = nullptr;
cl_mem ocl_mask_array = nullptr;
cl_mem ocl_cost_array = nullptr;
cl_mem ocl_updating_cost_array = nullptr;

cl_kernel initialize_buffer_kernel = nullptr;
cl_kernel sssp_kernel_1 = nullptr;
cl_kernel sssp_kernel_2 = nullptr;

int *source_vertices = nullptr;
float *results = nullptr;

GraphData graph;
int num_vertices = 1000;
int num_edges_per_vertex = 256;
int num_sources = 256;

/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // function declaration
    void generateRandomGraph( GraphData *graph, int num_vertices, int neighbors_per_vertex);
    void run_Dijkstra( cl_context gpu_context, cl_device_id device_id, GraphData *graph, int *source_vertices, float *out_result_costs, int num_results);
    cl_device_id get_max_flops_device( cl_context ocl_context);

    // variable declaration
    cl_int ocl_err;

    // code
        /******** Initialize OpenCL ***********/
    ocl_context = CreateContext( CL_DEVICE_TYPE_GPU);
    if( ocl_context == nullptr)
    {
        std::cerr << "CreateContext() Failed.";
        cleanup();
        return EXIT_FAILURE;
    }

    // Allocate memory for arrays
    generateRandomGraph( &graph, num_vertices, num_edges_per_vertex);

    std::cout << "Vertex Count : " << graph.vertex_count << "\n";
    std::cout << "Edge Count  : " << graph.edge_count << "\n";

    source_vertices = ( int*) malloc( sizeof( int) * num_sources);
    for( int i = 0; i < num_sources; ++i)
    {
        source_vertices[i] = i % graph.vertex_count;
    }

    results = (float*) malloc( sizeof( float) * num_sources * graph.vertex_count);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    run_Dijkstra( ocl_context, get_max_flops_device( ocl_context), &graph, source_vertices, results, num_sources);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double>  elapsed_seconds = end - start;
    std::cout << "Time Required : " << elapsed_seconds.count() << "s" << std::endl;

    cleanup();

    return 0;
}

/**
 * void generateRandomGraph()
*/
void generateRandomGraph( GraphData *graph, int num_vertices, int neighbors_per_vertex)
{
    // code
    graph->vertex_count = num_vertices;
    graph->p_vertex_array = (int*) malloc( graph->vertex_count * sizeof( int));
    
    graph->edge_count = num_vertices * neighbors_per_vertex;
    graph->p_edge_array = ( int *) malloc( graph->edge_count * sizeof( int));
    graph->p_weight_array = ( float*) malloc( graph->edge_count * sizeof( float));

    for( int i = 0; i < graph->vertex_count; ++i)
    {
        graph->p_vertex_array[i] = i * neighbors_per_vertex;
    }

    for( int i = 0; i < graph->edge_count; ++i)
    {
        graph->p_edge_array[i] = rand() % graph->vertex_count;
        graph->p_weight_array[i] = (float)(rand() % 1000) / 1000.0f;
    }
}

/**
 * get_max_flops_device()
 */
cl_device_id get_max_flops_device( cl_context ocl_context)
{
    // variable declaration
    size_t sz_param_data_bytes;
    cl_device_id *ocl_devices;

    // code
    clGetContextInfo( ocl_context, CL_CONTEXT_DEVICES, 0, nullptr, &sz_param_data_bytes);
    ocl_devices = ( cl_device_id *) malloc( sz_param_data_bytes);

    size_t device_count = sz_param_data_bytes / sizeof( cl_device_id);

    clGetContextInfo( ocl_context, CL_CONTEXT_DEVICES, sz_param_data_bytes, ocl_devices, nullptr);

    cl_device_id max_flops_device = ocl_devices[0];
    int max_flops = 0;

    size_t current_device = 0;

    // CL_DEVICE_MAX_COMPUTE_UNITS
    cl_uint compute_units;
    clGetDeviceInfo( ocl_devices[ current_device], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof( compute_units), &compute_units, nullptr);

    // CL_DEVICE_MAX_CLOCK_FREQUENCY
    cl_uint clock_frequency;
    clGetDeviceInfo( ocl_devices[ current_device], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, nullptr);

    max_flops = compute_units * clock_frequency;

    std::cout << "Device (" << current_device << ") => ComputeUnits: " << compute_units << ", ClockFrequency: " << clock_frequency << ", MaxFlops: " << max_flops << "\n";
    ++current_device;

    while( current_device < device_count)
    {
        // CL_DEVICE_MAX_COMPUTE_UNITS
        cl_uint compute_units;
        clGetDeviceInfo( ocl_devices[ current_device], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof( compute_units), &compute_units, nullptr);

        // CL_DEVICE_MAX_CLOCK_FREQUENCY
        cl_uint clock_frequency;
        clGetDeviceInfo( ocl_devices[ current_device], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, nullptr);

        int flops = compute_units * clock_frequency;

        std::cout << "Device (" << current_device << ") => ComputeUnits: " << compute_units << ", ClockFrequency: " << clock_frequency << ", MaxFlops: " << flops << "\n";

        if( flops > max_flops)
        {
            max_flops = flops;
            max_flops_device = ocl_devices[current_device];
        }

        ++current_device;
    }

    return max_flops_device;
}

/**
 * @brief run_Dijkstra()
 *          Run Dijkstra's shortest path on the GraphData provided to this function.
 *   This function will compute the shortest-path distance from
 *      source_vertices[n] -> end_vertices[n]
 *   and store the cost in out_result_costs[n].
 *   The number of results it will compute is given by num_results.
 * 
 *   This function will run the algorithm on a single GPU.
 * 
 * @param gpu_context      : Current GPU context, must be created by called.
 * 
 * @param device_id        : The device ID on which to run the kernel. This can be determined externally by the
 *                           caller or the multi-GPU version will automatically split the work across devices.
 * 
 * @param graph            : Containing the vertex, edge, and weight array for the input graph.
 * 
 * @param start_vertices   : Indices into the vertex array from which to start the search.
 * 
 * @param out_result_costs : A pre-allocated array where the results for each shortest-path search will be written.
 *                           This must be sized ( num_results * graph->num_vertices).
 * 
 * @param num_results      : Should be the size of all three passed in arrays.
 *                          
 */
void run_Dijkstra(
    cl_context context, cl_device_id device_id,
    GraphData *graph, int *source_vertices, float *out_result_costs, int num_results)
{
    // function declaration
    void allocateOCLBuffers( cl_context context, cl_command_queue command_queue, GraphData *graph,
                             cl_mem *vertex_array, cl_mem *edge_array, cl_mem *weight_array,
                             cl_mem *mask_array, cl_mem *cost_array, cl_mem *updating_cost_array,
                             size_t global_work_size);

    void initialize_OCL_buffers( cl_command_queue command_queue, cl_kernel initialize_kernel, GraphData *graph, size_t max_workgroup_size);
    bool is_maskArrayEmpty( int *mask_array, int count);
    int roundWorkSize( int group_size, int global_size);

    // code
        // create command queue
    cl_int err_num;

    ocl_command_queue = clCreateCommandQueue( context, device_id, 0, &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

        // program handle
    ocl_program = CreateProgram( context, device_id, "dijkstra.cl");
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

        // get the max workgroup size
    size_t max_workgroup_size;
    err_num = clGetDeviceInfo( device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof( size_t), &max_workgroup_size, nullptr);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    std::cout << "MAX_WORKGROUP_SIZE: " << max_workgroup_size << "\n";
    std::cout << "Computing '" << num_results << "' results." << "\n";

    // Set no. of work group and total in 1 dimensional range
    size_t local_work_size = max_workgroup_size;
    size_t global_work_size = roundWorkSize( local_work_size, graph->vertex_count);

    // allocate buffers in device memory
    allocateOCLBuffers( context, ocl_command_queue, graph, &ocl_vertex_array, &ocl_edge_array, &ocl_weight_array,
        &ocl_mask_array, &ocl_cost_array, &ocl_updating_cost_array, global_work_size);

    // create the kernels
        // Initialize kernel
    initialize_buffer_kernel = clCreateKernel( ocl_program, "initialize_buffers", &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    err_num |= clSetKernelArg( initialize_buffer_kernel, 0, sizeof(cl_mem), &ocl_mask_array);
    err_num |= clSetKernelArg( initialize_buffer_kernel, 1, sizeof(cl_mem), &ocl_cost_array);
    err_num |= clSetKernelArg( initialize_buffer_kernel, 2, sizeof(cl_mem), &ocl_updating_cost_array);
     // argument 3 set later
    err_num |= clSetKernelArg( initialize_buffer_kernel, 4, sizeof(int), &(graph->vertex_count));

        // kernel 1
    sssp_kernel_1 = clCreateKernel( ocl_program, "sssp_kernel_1", &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    err_num |= clSetKernelArg( sssp_kernel_1, 0, sizeof( cl_mem), &ocl_vertex_array);
    err_num |= clSetKernelArg( sssp_kernel_1, 1, sizeof( cl_mem), &ocl_edge_array);
    err_num |= clSetKernelArg( sssp_kernel_1, 2, sizeof( cl_mem), &ocl_weight_array);
    err_num |= clSetKernelArg( sssp_kernel_1, 3, sizeof( cl_mem), &ocl_mask_array);
    err_num |= clSetKernelArg( sssp_kernel_1, 4, sizeof( cl_mem), &ocl_cost_array);
    err_num |= clSetKernelArg( sssp_kernel_1, 5, sizeof( cl_mem), &ocl_updating_cost_array);
    err_num |= clSetKernelArg( sssp_kernel_1, 6, sizeof( int), &graph->vertex_count);
    err_num |= clSetKernelArg( sssp_kernel_1, 7, sizeof( int), &graph->edge_count);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

        // kernel 2
    sssp_kernel_2 = clCreateKernel( ocl_program, "sssp_kernel_2", &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    err_num |= clSetKernelArg( sssp_kernel_2, 0, sizeof( cl_mem), &ocl_vertex_array);
    err_num |= clSetKernelArg( sssp_kernel_2, 1, sizeof( cl_mem), &ocl_edge_array);
    err_num |= clSetKernelArg( sssp_kernel_2, 2, sizeof( cl_mem), &ocl_weight_array);
    err_num |= clSetKernelArg( sssp_kernel_2, 3, sizeof( cl_mem), &ocl_mask_array);
    err_num |= clSetKernelArg( sssp_kernel_2, 4, sizeof( cl_mem), &ocl_cost_array);
    err_num |= clSetKernelArg( sssp_kernel_2, 5, sizeof( cl_mem), &ocl_updating_cost_array);
    err_num |= clSetKernelArg( sssp_kernel_2, 6, sizeof( int), &graph->vertex_count);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    int *mask_array_host = (int *) malloc( sizeof( int) * graph->vertex_count);

    for( int i = 0; i < num_results; ++i)
    {
        err_num |= clSetKernelArg( initialize_buffer_kernel, 3, sizeof( int), &source_vertices[i]);
        CL_CHECK_ERROR( err_num, CL_SUCCESS);

        // initialize mask array to false, C and U to infinity
        initialize_OCL_buffers( ocl_command_queue, initialize_buffer_kernel, graph, max_workgroup_size);

        // read mask array from device -> host
        cl_event read_done_event;
        err_num = clEnqueueReadBuffer( ocl_command_queue, ocl_mask_array, CL_FALSE, 0, sizeof(int) * graph->vertex_count,
                                       mask_array_host, 0, nullptr, &read_done_event);
        CL_CHECK_ERROR( err_num, CL_SUCCESS);

        clWaitForEvents( 1, &read_done_event);

        while( !is_maskArrayEmpty( mask_array_host, graph->vertex_count))
        {
            // In order to improve performance, we run some number of iterations
            // without reading the results. This might result in running more iterations
            // than necessary at times, but it will in most cases be faster because we are
            // doing less stalling of the GPU waiting for results.
            for( int asyncIter = 0; asyncIter < NUM_ASYNCHRONOUS_ITERATIONS; asyncIter++)
            {
                size_t localWorkSize = max_workgroup_size;
                size_t globalWorkSize = roundWorkSize( local_work_size, graph->vertex_count);

                // execute the kernel
                err_num = clEnqueueNDRangeKernel( ocl_command_queue, sssp_kernel_1, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
                CL_CHECK_ERROR( err_num, CL_SUCCESS);

                err_num = clEnqueueNDRangeKernel( ocl_command_queue, sssp_kernel_2, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
                CL_CHECK_ERROR( err_num, CL_SUCCESS);
            }

            err_num = clEnqueueReadBuffer( ocl_command_queue, ocl_mask_array, CL_FALSE, 0, sizeof( int) * graph->vertex_count, mask_array_host, 0, nullptr, &read_done_event);
            CL_CHECK_ERROR( err_num, CL_SUCCESS);

            clWaitForEvents( 1, &read_done_event);
        }

        // copy the result back
        err_num = clEnqueueReadBuffer( ocl_command_queue, ocl_cost_array, CL_FALSE, 0, sizeof(float) * graph->vertex_count,
                                       &out_result_costs[i * graph->vertex_count], 0, nullptr, &read_done_event);
        CL_CHECK_ERROR( err_num, CL_SUCCESS);
        clWaitForEvents( 1, &read_done_event);
    }

    free( mask_array_host);

}

/**
 * allocateOCLBuffers()
 */
void allocateOCLBuffers(
    cl_context context, cl_command_queue command_queue, GraphData *graph,
    cl_mem *vertex_array, cl_mem *edge_array, cl_mem *weight_array,
    cl_mem *mask_array, cl_mem *cost_array, cl_mem *updating_cost_array,
    size_t global_work_size
)
{
    // variable declaration
    cl_int err_num;
    cl_mem host_vertex_array_buffer = nullptr;
    cl_mem host_edge_array_buffer = nullptr;
    cl_mem host_weight_array_buffer = nullptr;

    // code
        // First, need to create OpenCL host buffers that can be copied to device buffers
    host_vertex_array_buffer = clCreateBuffer(
                                    context, CL_MEM_COPY_HOST_PTR | CL_MEM_ALLOC_HOST_PTR,
                                    sizeof( int) * graph->vertex_count, graph->p_vertex_array, &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);


    host_edge_array_buffer = clCreateBuffer(
                                    context, CL_MEM_COPY_HOST_PTR | CL_MEM_ALLOC_HOST_PTR,
                                    sizeof( int) * graph->edge_count, graph->p_edge_array, &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);


    host_weight_array_buffer = clCreateBuffer(
                                    context, CL_MEM_COPY_HOST_PTR | CL_MEM_ALLOC_HOST_PTR,
                                    sizeof( int) * graph->edge_count, graph->p_weight_array, &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

        // Now create all of the GPU buffers
    *vertex_array = clCreateBuffer( context, CL_MEM_READ_ONLY, sizeof( int) * global_work_size, nullptr, &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    *edge_array = clCreateBuffer( context, CL_MEM_READ_ONLY, sizeof( int) * graph->edge_count, nullptr, &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    *weight_array = clCreateBuffer( context, CL_MEM_READ_ONLY, sizeof( float) * graph->edge_count, nullptr, &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    *mask_array = clCreateBuffer( context, CL_MEM_READ_WRITE, sizeof( int) * global_work_size, nullptr, &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    *cost_array = clCreateBuffer( context, CL_MEM_READ_WRITE, sizeof(float) * global_work_size, nullptr, &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    *updating_cost_array = clCreateBuffer( context, CL_MEM_READ_WRITE, sizeof(float) * global_work_size, nullptr, &err_num);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

        // Now queue up the data to be copied to the device
    err_num = clEnqueueCopyBuffer( command_queue, host_vertex_array_buffer, *vertex_array, 0, 0,
                                    sizeof(int) * graph->vertex_count, 0, nullptr, nullptr);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    err_num = clEnqueueCopyBuffer( command_queue, host_edge_array_buffer, *edge_array, 0, 0,
                                    sizeof(int) * graph->edge_count, 0, nullptr, nullptr);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    err_num = clEnqueueCopyBuffer( command_queue, host_weight_array_buffer, *weight_array, 0, 0,
                                    sizeof(int) * graph->edge_count, 0, nullptr, nullptr);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);

    clReleaseMemObject( host_vertex_array_buffer);
    clReleaseMemObject( host_edge_array_buffer);
    clReleaseMemObject( host_weight_array_buffer);
}

/**
 * initialize_OCL_buffers()
 */
void initialize_OCL_buffers( cl_command_queue command_queue, cl_kernel initialize_kernel, GraphData *graph, size_t max_workgroup_size)
{
    // functional declaration
    int roundWorkSize( int group_size, int global_size);

    // variable declaration
    cl_int err_num;

    // code
        // set no. of work items in work group and total in 1 dimesional range
    size_t local_work_size = max_workgroup_size;
    size_t global_work_size = roundWorkSize( local_work_size, graph->vertex_count);

    err_num = clEnqueueNDRangeKernel( command_queue, initialize_kernel, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
    CL_CHECK_ERROR( err_num, CL_SUCCESS);
}

/**
 *is_maskArrayEmpty()
 */
bool is_maskArrayEmpty( int *mask_array, int count)
{
    // code
    for( int i = 0; i < count; ++i)
    {
        if( mask_array[i] == 1)
        {
            return false;
        }
    }

    return true;
}

/**
 * roundWorkSize()
 */
int roundWorkSize( int group_size, int global_size)
{
    // code
    int remainder = global_size % group_size;
    if( remainder == 0)
    {
        return global_size;
    }
    
    return global_size + group_size - remainder;
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

    RELEASE_CL_OBJECT( ocl_vertex_array, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_edge_array, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_weight_array, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_mask_array, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_cost_array, clReleaseMemObject);
    RELEASE_CL_OBJECT( ocl_updating_cost_array, clReleaseMemObject);

    RELEASE_CL_OBJECT( initialize_buffer_kernel, clReleaseKernel);
    RELEASE_CL_OBJECT( sssp_kernel_1, clReleaseKernel);
    RELEASE_CL_OBJECT( sssp_kernel_2, clReleaseKernel);

    RELEASE_CL_OBJECT( source_vertices, free);
    RELEASE_CL_OBJECT( results, free);

    graph.release();
}

