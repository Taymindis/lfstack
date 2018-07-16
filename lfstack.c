#include <stdlib.h>
#include <errno.h>
#include "lfstack.h"

static lfstack_node_t *
pop_(lfstack_t *lfstack) {
    lfstack_cas_node_t curr, prev;
    __atomic_load(&lfstack->head, &curr, __ATOMIC_ACQUIRE);
    do {
        if ( curr.aba == lfstack->head.aba /*__sync_bool_compare_and_swap(&lfstack->head.aba, curr.aba, curr.aba)*/) {
            prev.node = curr.node->prev;
            prev.aba = curr.aba + 1;
        } else {
            return NULL;
        }
    } while (!__atomic_compare_exchange(&lfstack->head, &curr, &prev, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

    return curr.node;
}

static int
push_(lfstack_t *lfstack, void* value) {
    lfstack_cas_node_t curr, next;
    lfstack_node_t *node = malloc(sizeof(lfstack_node_t));
    node->value = value;
    next.node = node;
    __atomic_load(&lfstack->head, &curr, __ATOMIC_ACQUIRE);

    do {
        next.aba = curr.aba + 1;
        node->prev = curr.node;
    } while (!__atomic_compare_exchange(&lfstack->head, &curr, &next, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
    return 1;
}

int
lfstack_init(lfstack_t *lfstack) {
    lfstack_node_t *base = malloc(sizeof(lfstack_node_t));
    if (base == NULL) {
        return errno;
    }
    base->value = NULL;
    base->prev = NULL;

    lfstack->head.aba = 0;
    lfstack->head.node = base;

    lfstack->size = 0;
    return 1;
}

void
lfstack_clear(lfstack_t *lfstack) {
    void* p;
    while ( (p = lfstack_pop(lfstack)) ) {
        free(p);
    }
    free(lfstack->head.node);
    lfstack->size = 0;
}

int
lfstack_push(lfstack_t *lfstack, void *value) {
    if (push_(lfstack, value)) {
        __atomic_fetch_add(&lfstack->size, 1, __ATOMIC_RELAXED);
        return 1;
    }
    return 0;
}

void*
lfstack_pop(lfstack_t *lfstack) {
    lfstack_node_t *node;
    void *v;
    if ( __atomic_load_n(&lfstack->size, __ATOMIC_ACQUIRE) && (node = pop_(lfstack)) ) {
        __atomic_fetch_add(&lfstack->size, -1, __ATOMIC_RELAXED);
        v = node->value;
        free(node);
        return v;
    }
    return NULL;
}

size_t
lfstack_size(lfstack_t *lfstack) {
    return __atomic_load_n(&lfstack->size, __ATOMIC_ACQUIRE);
}
