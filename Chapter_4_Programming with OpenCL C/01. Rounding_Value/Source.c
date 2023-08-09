/**
 * @author : Vijaykumar Dangi
 * @date   : 19-July-2023
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // code
    float a = 0.5f;
    float b = a - nextafterf( a, (float) -INFINITY);   // a - 1 ulp

    //printf( "%f, %f\n", a, nextafterf(a, 1));

    printf(
        "a = %8x (%u), b = %8x (%u)\n",
        *(unsigned int *)&a,
        *(unsigned int *)&a,
        *(unsigned int *)&b,
        *(unsigned int *)&b
    );

    printf("(int)(a + 0.5f) = %d \n", (int)(a + 0.5f));
    printf("(int)(b + 0.5f) = %d \n", (int)(b + 0.5f));

    return 0;
}

