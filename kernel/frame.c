#include <stddef.h>
#include <stdint.h>

#include "frame.h"
#include "heap.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#define MULTIBOOT_FLAG_MMAP (1u << 6)

#define FRAME_SIZE 4096u
#define MAX_FRAMES (1024u * 1024u)
#define FRAME_BITMAP_BYTES (MAX_FRAMES / 8u)

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed)) multiboot_info_t;

typedef struct multiboot_mmap_entry {
    uint32_t size;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t len_low;
    uint32_t len_high;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

extern uint8_t __kernel_end;

static uint8_t frame_bitmap[FRAME_BITMAP_BYTES];
static uint32_t frame_total_frames;
static uint32_t frame_free_frames;
static uint32_t frame_last_index;
static int frame_ready;

static void frame_set(uint32_t frame) {
    frame_bitmap[frame / 8u] |= (uint8_t)(1u << (frame % 8u));
}

static void frame_clear(uint32_t frame) {
    frame_bitmap[frame / 8u] &= (uint8_t)~(1u << (frame % 8u));
}

static int frame_test(uint32_t frame) {
    return (frame_bitmap[frame / 8u] & (uint8_t)(1u << (frame % 8u))) != 0u;
}

static uint32_t align_down(uint32_t value, uint32_t align) {
    return value & ~(align - 1u);
}

static uint32_t align_up_u32(uint32_t value, uint32_t align) {
    return (value + align - 1u) & ~(align - 1u);
}

static void frame_range_set(uint32_t start_addr, uint32_t length, int used) {
    uint32_t start = align_down(start_addr, FRAME_SIZE) / FRAME_SIZE;
    uint32_t end = align_up_u32(start_addr + length, FRAME_SIZE) / FRAME_SIZE;

    if (end > frame_total_frames) {
        end = frame_total_frames;
    }

    for (uint32_t i = start; i < end; i++) {
        if (used) {
            frame_set(i);
        } else {
            frame_clear(i);
        }
    }
}

void frame_reserve_range(uint32_t start_addr, uint32_t length) {
    frame_range_set(start_addr, length, 1);
}

void frame_release_range(uint32_t start_addr, uint32_t length) {
    frame_range_set(start_addr, length, 0);
}

static void frame_recount(void) {
    uint32_t free_count = 0;

    for (uint32_t i = 0; i < frame_total_frames; i++) {
        if (!frame_test(i)) {
            free_count++;
        }
    }

    frame_free_frames = free_count;
}

void frame_init(uint32_t multiboot_magic, uint32_t multiboot_addr) {
    const multiboot_info_t* mbi;
    uint32_t max_end = 0;

    frame_ready = 0;
    frame_total_frames = 0;
    frame_free_frames = 0;
    frame_last_index = 0;

    for (uint32_t i = 0; i < FRAME_BITMAP_BYTES; i++) {
        frame_bitmap[i] = 0xFFu;
    }

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        return;
    }

    mbi = (const multiboot_info_t*)(uintptr_t)multiboot_addr;
    if ((mbi->flags & MULTIBOOT_FLAG_MMAP) == 0u) {
        return;
    }

    uintptr_t current = (uintptr_t)mbi->mmap_addr;
    uintptr_t end = current + mbi->mmap_length;

    while (current < end) {
        const multiboot_mmap_entry_t* entry = (const multiboot_mmap_entry_t*)current;

        if (entry->len_high == 0u) {
            uint32_t region_end = entry->addr_low + entry->len_low;
            if (region_end > max_end) {
                max_end = region_end;
            }

            if (entry->type == 1u) {
                frame_release_range(entry->addr_low, entry->len_low);
            }
        }

        current += (uintptr_t)entry->size + sizeof(entry->size);
    }

    frame_total_frames = align_up_u32(max_end, FRAME_SIZE) / FRAME_SIZE;
    if (frame_total_frames > MAX_FRAMES) {
        frame_total_frames = MAX_FRAMES;
    }

    frame_reserve_range(0u, 0x100000u);
    frame_reserve_range(0x100000u, (uint32_t)((uintptr_t)&__kernel_end - 0x100000u));
    frame_reserve_range(heap_start_addr(), heap_end_addr() - heap_start_addr());
    frame_reserve_range(multiboot_addr, (uint32_t)sizeof(multiboot_info_t));
    frame_reserve_range(mbi->mmap_addr, mbi->mmap_length);

    frame_recount();
    frame_ready = 1;
}

uint32_t frame_alloc(void) {
    if (!frame_ready || frame_free_frames == 0u) {
        return 0u;
    }

    for (uint32_t i = 0; i < frame_total_frames; i++) {
        uint32_t index = (frame_last_index + i) % frame_total_frames;

        if (!frame_test(index)) {
            frame_set(index);
            frame_free_frames--;
            frame_last_index = index + 1u;
            return index * FRAME_SIZE;
        }
    }

    return 0u;
}

void frame_free(uint32_t addr) {
    uint32_t frame = addr / FRAME_SIZE;

    if (!frame_ready || frame >= frame_total_frames) {
        return;
    }

    if (frame_test(frame)) {
        frame_clear(frame);
        frame_free_frames++;
    }
}

uint32_t frame_total(void) {
    return frame_total_frames;
}

uint32_t frame_free_count(void) {
    return frame_free_frames;
}

uint32_t frame_used_count(void) {
    if (!frame_ready) {
        return 0u;
    }

    return frame_total_frames - frame_free_frames;
}

int frame_is_ready(void) {
    return frame_ready;
}