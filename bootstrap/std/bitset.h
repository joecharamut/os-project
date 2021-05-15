#ifndef OS_BITSET_H
#define OS_BITSET_H

#include <stdbool.h>
#include "types.h"

typedef struct bitset_node {
    u32 value;
    struct bitset_node *next;
} bitset_node_t;

typedef struct bitset {
    bitset_node_t *values;
} bitset_t;

bitset_t *bitset_new(int size);
void bitset_set(bitset_t *this, int bit);
void bitset_clear(bitset_t *this, int bit);
bool bitset_test(bitset_t *this, int bit);

#endif //OS_BITSET_H
