#include <stddef.h>
#include <stdint.h>

#include "memory.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#define MULTIBOOT_FLAG_MEM (1u << 0)
#define MULTIBOOT_FLAG_MMAP (1u << 6)

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

static uint32_t g_memory_total_kb;
static uint32_t g_memory_available_kb;
static uint32_t g_memory_region_count;
static int g_memory_ready;

void memory_init(uint32_t multiboot_magic, uint32_t multiboot_addr) {
    const multiboot_info_t* mbi;

    g_memory_total_kb = 0;
    g_memory_available_kb = 0;
    g_memory_region_count = 0;
    g_memory_ready = 0;

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        return;
    }

    mbi = (const multiboot_info_t*)(uintptr_t)multiboot_addr;

    if ((mbi->flags & MULTIBOOT_FLAG_MMAP) != 0u) {
        uintptr_t current = (uintptr_t)mbi->mmap_addr;
        uintptr_t end = current + mbi->mmap_length;

        while (current < end) {
            const multiboot_mmap_entry_t* entry = (const multiboot_mmap_entry_t*)current;

            if (entry->len_high == 0u) {
                uint32_t kb = entry->len_low / 1024u;

                g_memory_total_kb += kb;
                if (entry->type == 1u) {
                    g_memory_available_kb += kb;
                }
            }

            g_memory_region_count++;
            current += (uintptr_t)entry->size + sizeof(entry->size);
        }

        g_memory_ready = 1;
        return;
    }

    if ((mbi->flags & MULTIBOOT_FLAG_MEM) != 0u) {
        g_memory_total_kb = mbi->mem_lower + mbi->mem_upper;
        g_memory_available_kb = mbi->mem_upper;
        g_memory_region_count = 1;
        g_memory_ready = 1;
    }
}

uint32_t memory_total_kb(void) {
    return g_memory_total_kb;
}

uint32_t memory_available_kb(void) {
    return g_memory_available_kb;
}

uint32_t memory_region_count(void) {
    return g_memory_region_count;
}

int memory_is_ready(void) {
    return g_memory_ready;
}