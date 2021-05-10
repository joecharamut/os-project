#include <debug/panic.h>
#include "math.h"

double pow(double base, double power) {
    if (power == 0) return 1;
    if (power < 0) return ((double) 1.0) / pow(base, -power);

    u64 ipart = (u64) power;
    u64 dpart = (u64) ((power - ipart) * 100000);

    double ivalue = base;
    for (u64 i = 1; i < ipart; i++) {
        ivalue *= base;
    }

    double dvalue = 0;
    if (dpart > 0) {
        PANIC("cant do decimal powers yet");
    }

    return ivalue;
}

int abs(int val) {
    if (val < 0) return -val;
    return val;
}
