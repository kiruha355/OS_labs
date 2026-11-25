#ifndef METHODS_H
#define METHODS_H

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static inline float integrate(float A, float B, float e, double (*area)(double, double))
{
    if (!isfinite(A) || !isfinite(B) || !isfinite(e) || A > B || e <= 0)
        return NAN;

    double sum = 0;
    for (double left = A; left < B;) {
        double right = fmin(left + e, B);
        if (right <= left)
            return NAN;
        sum += area(left, right);
        left = right;
    }
    return fabs(sum) <= FLT_MAX ? (float)sum : NAN;
}

static inline char *to_base(long x, unsigned int base)
{
    char buffer[sizeof(long) * CHAR_BIT + 2];
    size_t start = sizeof(buffer) - 1;
    buffer[start] = '\0';

    unsigned long number = (unsigned long)x;
    if (x < 0) {
        number = 0UL - number;
    }

    if (number == 0) {
        --start;
        buffer[start] = '0';
    }
    while (number > 0) {
        --start;
        buffer[start] = (char)('0' + number % base);
        number /= base;
    }
    if (x < 0) {
        --start;
        buffer[start] = '-';
    }

    size_t size = sizeof(buffer) - start;
    char *result = malloc(size);
    if (result != NULL)
        memcpy(result, &buffer[start], size);
    return result;
}

#endif
