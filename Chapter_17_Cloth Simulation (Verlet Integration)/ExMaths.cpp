//
//Extra Math Function
//
//  ExMath.cpp
//

#include <stdio.h>
#include "ExMaths.h"

/**
* @brief WrapInt():- wrap "value" between "min" and "max"
*/
int WrapInt( int value, int min, int max)
{
    int diff = max - min;

    while( value > max)
    {
        value -= diff;
    }
    while( value < min)
    {
        value += diff;
    }

    return( value);
}

