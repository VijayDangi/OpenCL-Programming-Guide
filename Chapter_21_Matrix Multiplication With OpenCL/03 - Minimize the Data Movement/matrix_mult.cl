
#define DIMENSION 1000

__kernel void mat_mul(
    const int m_dimension, const int n_dimension, const int p_dimension,
    __global float *A_matrix, __global float *B_matrix, __global float *C_matrix
)
{
    // code
    int k, j;
    int i = get_global_id(0);
    float Awork[1000];

    float temp;

    if( i < n_dimension)
    {
        for( k = 0; k < DIMENSION; ++k)
        {
            Awork[k] = A_matrix[i * n_dimension + k];
        }

        for( j = 0; j < m_dimension; ++j)
        {
            temp = 0.0;
            for( k = 0; k < p_dimension; ++k)
            {
                temp += Awork[i * n_dimension + k] * B_matrix[ k * p_dimension + j];
            }

            C_matrix[i * n_dimension + j] = temp;
        }
    }
}
