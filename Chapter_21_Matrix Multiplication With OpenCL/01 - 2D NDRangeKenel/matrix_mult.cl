
__kernel void mat_mul(
    const int m_dimension, const int n_dimension, const int p_dimension,
    __global float *A_matrix, __global float *B_matrix, __global float *C_matrix
)
{
    // code
    int k;
    int i = get_global_id(0);
    int j = get_global_id(1);

    float temp;

    if( (i < n_dimension) && (j < m_dimension))
    {
        temp = 0.0;
        for( k = 0; k < p_dimension; ++k)
        {
            temp += A_matrix[i * n_dimension + k] * B_matrix[ k * p_dimension + j];
        }

        C_matrix[i * n_dimension + j] = temp;
    }
}
