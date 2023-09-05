/**
 * Implementation of Dijkstra's Single-Source Shortest Path (SSSP) algorithm on the GPU.
 * 
 * The basis of this implementation is the paper:
 *
 *      "Accelerating large graph algorithms on the GPU using CUBA"
 *                  - Parwan Harish and P.J. Narayanan
 */

/**
 * initialize_buffers()
 */
__kernel void initialize_buffers(
    __global int *mask_array, __global float *cost_array, __global float *updating_cost_array,
    int source_vertex, int vertex_count)
{
    // access thread id
    int tid = get_global_id(0);

    if( source_vertex == tid)
    {
        mask_array[tid] = 1;
        cost_array[tid] = 0.0;
        updating_cost_array[tid] = 0.0;
    }
    else
    {
        mask_array[tid] = 0;
        cost_array[tid] = FLT_MAX;
        updating_cost_array[tid] = FLT_MAX;
    }
}

/**
 * sssp_kernel_1() :-
 *      Part 1 of the Kernel from Algorithm 4 in the paper
 */
__kernel void sssp_kernel_1(
    __global int *vertex_array, __global int *edge_array, __global float *weight_array,
    __global int *mask_array, __global float *cost_array, __global float *updating_cost_array,
    int vertex_count, int edge_count
)
{
    // access thread id
    int tid = get_global_id(0);

    if( mask_array[tid] != 0)
    {
        mask_array[tid] = 0;

        int edge_start = vertex_array[tid];
        int edge_end;

        if( (tid + 1) < vertex_count)
        {
            edge_end = vertex_array[tid + 1];
        }
        else
        {
            edge_end = edge_count;
        }

        for( int edge = edge_start; edge < edge_end; ++edge)
        {
            int nid = edge_array[edge];

            // One note here:
            //      Whereas the paper specified weight_array[nid], Dan Ginsburg found that
            //      the correct thing to do was weight_array[edge]. Dan Ginsburg think this was a
            //      typo in the paper.
            if( updating_cost_array[nid] > (cost_array[tid] + weight_array[edge]))
            {
                updating_cost_array[nid] = ( cost_array[tid] + weight_array[edge]);
            }
        }
    }
}


/**
 * sssp_kernel_2() :-
 *      Part 2 of the Kernel from Algorithm 5 in the paper
 */
__kernel void sssp_kernel_2(
    __global int *vertex_array, __global int *edge_array, __global float *weight_array,
    __global int *mask_array, __global float *cost_array, __global float *updating_cost_array,
    int vertex_count
)
{
    // access thread id
    int tid = get_global_id(0);

    if( cost_array[tid] > updating_cost_array[tid])
    {
        cost_array[tid] = updating_cost_array[tid];
        mask_array[tid] = 1;
    }

    updating_cost_array[tid] = cost_array[tid];
}
