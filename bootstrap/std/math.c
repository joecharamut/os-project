#include "math.h"

double pow(double base, double exponent) {
    double result;
    asm volatile (
    "fldl %2            \n" // load exponent
    "fldl %1            \n" // load base
    "fyl2x              \n" // st(0) = y*log2(x)
    "fld %%st           \n" // copy st(0)
    "frndint            \n" // round it
    "fsub %%st, %%st(1) \n" // get just the fraction part in st(1)
    "fxch %%st(1)       \n" // exchange st(0) and st(1)
    "f2xm1              \n" // st(0) = 2^x minus 1
    "fld1               \n" // load 1
    "faddp              \n" // st(0) = st(0) + st(1)
    "fscale             \n" // st(0) = st(0) * 2^st(1)
    "fstpl %0           \n" // store st(0) and pop
    : "=m" (result)
    : "m" (base), "m" (exponent)
    :
    );
    return result;
}

int abs(int val) {
    if (val < 0) return -val;
    return val;
}

double fabs(double val) {
    if (val < 0) return -val;
    return val;
}

double logb(double base, double x) {
    double result;
    asm volatile (
    "fld1                   \n" // st(1) = 1
    "fldl %1                \n" // st(0) = base
    "fyl2x                  \n" // st(0) = y * log2(x)
    "fld1                   \n" // st(1) = 1
    "fldl %2                \n" // st(0) = x
    "fyl2x                  \n" // st(0) = y * log2(x)
    "fdiv %%st(1), %%st(0)  \n" // st(0) = st(0) / st(1)
    "fstpl %0               \n" // pop st(0)
    : "=m" (result)
    : "m" (base), "m" (x)
    :
    );
    return result;
}
