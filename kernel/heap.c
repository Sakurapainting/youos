#include <stddef.h>
#include <stdint.h>

#include "heap.h"

#define HEAP_DEFAULT_SIZE (1024u * 1024u)
#define HEAP_DEFAULT_ALIGN 16u

extern uint8_t __kernel_end;

static uintptr_t heap_start;
static uintptr_t heap_end;
static uintptr_t heap_current;
static int heap_ready;

static uintptr_t align_up(uintptr_t value, uint32_t align) {
    if (align == 0u) {
        return value;
    }

    return (value + (uintptr_t)(align - 1u)) & ~(uintptr_t)(align - 1u);
}

void heap_init(void) {
    heap_start = align_up((uintptr_t)&__kernel_end, HEAP_DEFAULT_ALIGN);
    heap_end = heap_start + HEAP_DEFAULT_SIZE;
    heap_current = heap_start;
    heap_ready = 1;
}

void* heap_alloc(uint32_t size, uint32_t align) {
    uintptr_t current;
    uintptr_t next;

    if (!heap_ready || size == 0u) {
        return NULL;
    }

    if (align == 0u) {
        align = HEAP_DEFAULT_ALIGN;
    }

    current = align_up(heap_current, align);
    next = current + size;

    if (next > heap_end) {
        return NULL;
    }

    heap_current = next;
    return (void*)current;
}

uint32_t heap_used_bytes(void) {
    if (!heap_ready) {
        return 0u;
    }

    return (uint32_t)(heap_current - heap_start);
}

uint32_t heap_total_bytes(void) {
    if (!heap_ready) {
        return 0u;
    }

    return (uint32_t)(heap_end - heap_start);
}

int heap_is_ready(void) {
    return heap_ready;
}