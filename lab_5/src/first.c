#include "contracts.h"
#include "methods.h"

static double rectangle(double left, double right)
{
    return sin(left) * (right - left);
}

float SinIntegral(float A, float B, float e)
{
    return integrate(A, B, e, rectangle);
}

char *translation(long x)
{
    return to_base(x, 2);
}
