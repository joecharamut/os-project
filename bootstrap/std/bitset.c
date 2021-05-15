#include <mm/kmem.h>
#include "bitset.h"

bitset_t *bitset_new(int size) {
    bitset_t *set = kmalloc(sizeof(bitset_t));
    bitset_node_t *node = kmalloc(sizeof(bitset_node_t));

    set->values = node;

    int nodes = size / 32;
    for (int i = 0; i < nodes; i++) {
        node->next = kmalloc(sizeof(bitset_node_t));
        node = node->next;
    }

    return set;
}

void bitset_set(bitset_t *this, int bit) {
    int node_offset = bit / 32;
    int bit_offset = bit % 32;

    bitset_node_t *node = this->values;
    for (int i = 0; i < node_offset; i++) {
        node = node->next;
    }

    node->value |= (1 << bit_offset);
}

void bitset_clear(bitset_t *this, int bit) {
    int node_offset = bit / 32;
    int bit_offset = bit % 32;

    bitset_node_t *node = this->values;
    for (int i = 0; i < node_offset; i++) {
        node = node->next;
    }

    node->value &= ~(1 << bit_offset);
}

bool bitset_test(bitset_t *this, int bit) {
    int node_offset = bit / 32;
    int bit_offset = bit % 32;

    bitset_node_t *node = this->values;
    for (int i = 0; i < node_offset; i++) {
        node = node->next;
    }

    return node->value &= (1 << bit_offset);
}
