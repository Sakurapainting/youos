#include <stddef.h>
#include <stdint.h>

#include "heap.h"

#define HEAP_DEFAULT_SIZE (1024u * 1024u)
#define HEAP_DEFAULT_ALIGN 16u
#define HEAP_MIN_SPLIT (HEAP_DEFAULT_ALIGN + (uint32_t)sizeof(heap_block_t))

extern uint8_t __kernel_end;

typedef struct heap_block {
    uint32_t size;
    uint32_t used;
    struct heap_block* prev;
    struct heap_block* next;
} heap_block_t;

static uintptr_t heap_start;
static uintptr_t heap_end;
static heap_block_t* heap_head;
static uint32_t heap_total_payload;
static int heap_ready;

static uintptr_t align_up(uintptr_t value, uint32_t align) {
    if (align == 0u) {
        return value;
    }

    return (value + (uintptr_t)(align - 1u)) & ~(uintptr_t)(align - 1u);
}

static void heap_split_block(heap_block_t* block, uint32_t size) {
    uintptr_t split_addr;
    heap_block_t* new_block;

    if (block->size < size + HEAP_MIN_SPLIT) {
        return;
    }

    split_addr = (uintptr_t)block + sizeof(heap_block_t) + size;
    new_block = (heap_block_t*)split_addr;
    new_block->size = block->size - size - (uint32_t)sizeof(heap_block_t);
    new_block->used = 0;
    new_block->prev = block;
    new_block->next = block->next;

    if (block->next != NULL) {
        block->next->prev = new_block;
    }

    block->next = new_block;
    block->size = size;
}

static void heap_merge_with_next(heap_block_t* block) {
    heap_block_t* next = block->next;

    if (next == NULL || next->used) {
        return;
    }

    block->size += (uint32_t)sizeof(heap_block_t) + next->size;
    block->next = next->next;

    if (next->next != NULL) {
        next->next->prev = block;
    }
}

void heap_init(void) {
    heap_start = align_up((uintptr_t)&__kernel_end, HEAP_DEFAULT_ALIGN);
    heap_end = heap_start + HEAP_DEFAULT_SIZE;

    heap_head = (heap_block_t*)heap_start;
    heap_head->size = (uint32_t)(heap_end - heap_start - sizeof(heap_block_t));
    heap_head->used = 0;
    heap_head->prev = NULL;
    heap_head->next = NULL;

    heap_total_payload = heap_head->size;
    heap_ready = 1;
}

void* heap_alloc(uint32_t size, uint32_t align) {
    heap_block_t* block;
    uint32_t aligned_size;

    if (!heap_ready || size == 0u) {
        return NULL;
    }

    if (align == 0u || align > HEAP_DEFAULT_ALIGN) {
        align = HEAP_DEFAULT_ALIGN;
    }

    aligned_size = (uint32_t)align_up(size, HEAP_DEFAULT_ALIGN);

    block = heap_head;
    while (block != NULL) {
        if (!block->used && block->size >= aligned_size) {
            heap_split_block(block, aligned_size);
            block->used = 1;
            return (void*)((uintptr_t)block + sizeof(heap_block_t));
        }

        block = block->next;
    }

    return NULL;
}

void heap_free(void* ptr) {
    heap_block_t* block;

    if (!heap_ready || ptr == NULL) {
        return;
    }

    block = (heap_block_t*)((uintptr_t)ptr - sizeof(heap_block_t));
    block->used = 0;

    if (block->next != NULL && !block->next->used) {
        heap_merge_with_next(block);
    }

    if (block->prev != NULL && !block->prev->used) {
        heap_merge_with_next(block->prev);
    }
}

uint32_t heap_used_bytes(void) {
    if (!heap_ready) {
        return 0u;
    }

    heap_block_t* block = heap_head;
    uint32_t used = 0;

    while (block != NULL) {
        if (block->used) {
            used += block->size;
        }
        block = block->next;
    }

    return used;
}

uint32_t heap_total_bytes(void) {
    if (!heap_ready) {
        return 0u;
    }

    return heap_total_payload;
}

int heap_is_ready(void) {
    return heap_ready;
}