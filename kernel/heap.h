#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

void heap_init(void);
void* heap_alloc(uint32_t size, uint32_t align);
void heap_free(void* ptr);
uint32_t heap_used_bytes(void);
uint32_t heap_total_bytes(void);
int heap_is_ready(void);

#endif