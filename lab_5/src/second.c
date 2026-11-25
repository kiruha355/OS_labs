#include "contracts.h"
#include "methods.h"

static double trapezoid(double left, double right)
{
    return (sin(left) + sin(right)) * (right - left) / 2;
}

float SinIntegral(float A, float B, float e)
{
    return integrate(A, B, e, trapezoid);
}

char *translation(long x)
{
    return to_base(x, 3);
}
