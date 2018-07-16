#ifndef LFSTACK_H
#define LFSTACK_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lfstack_node_s{
	struct lfstack_node_s *prev;
	void * value;
} lfstack_node_t;

typedef struct lfstack_cas_node_s {
	uintptr_t aba;
	struct lfstack_node_s *node;
} lfstack_cas_node_t;


typedef struct {
	lfstack_cas_node_t head;
	size_t size;
} lfstack_t;

int   lfstack_init(lfstack_t *lfstack);
int   lfstack_push(lfstack_t *lfstack, void *value);
void *lfstack_pop(lfstack_t *lfstack);
void lfstack_clear(lfstack_t *lfstack);
size_t lfstack_size(lfstack_t *lfstack);

#ifdef __cplusplus
}
#endif

#endif
