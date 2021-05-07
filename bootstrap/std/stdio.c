#include "stdio.h"

void memset(void *ptr, u8 value, u32 count) {
    for (u32 i = 0; i < count; ++i) {
        *((u8 *) ptr) = value;
    }
}
