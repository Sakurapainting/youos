#include <stddef.h>
#include <stdint.h>

#include "frame.h"
#include "paging.h"

static uint32_t* paging_directory;
static int paging_ready;

static uint32_t paging_align_down(uint32_t value) {
    return value & ~(PAGE_SIZE - 1u);
}

static uint32_t paging_align_up(uint32_t value) {
    return (value + PAGE_SIZE - 1u) & ~(PAGE_SIZE - 1u);
}

static uint32_t* paging_table_ptr(uint32_t phys_addr) {
    return (uint32_t*)(uintptr_t)phys_addr;
}

void paging_init(void) {
    uint32_t directory_phys;
    uint32_t* directory;

    paging_ready = 0;
    paging_directory = NULL;

    directory_phys = frame_alloc();
    if (directory_phys == 0u) {
        return;
    }

    directory = paging_table_ptr(directory_phys);
    for (uint32_t i = 0; i < PAGE_ENTRIES; i++) {
        directory[i] = 0u;
    }

    paging_directory = directory;
    paging_ready = 1;
}

int paging_map_identity(uint32_t start_addr, uint32_t length, uint32_t flags) {
    uint32_t start;
    uint32_t end;

    if (!paging_ready || length == 0u) {
        return 0;
    }

    start = paging_align_down(start_addr);
    end = paging_align_up(start_addr + length);

    for (uint32_t addr = start; addr < end; addr += PAGE_SIZE) {
        uint32_t directory_index = addr >> 22;
        uint32_t table_index = (addr >> 12) & 0x3FFu;
        uint32_t table_phys = paging_directory[directory_index] & ~(PAGE_SIZE - 1u);
        uint32_t* table;

        if (table_phys == 0u) {
            table_phys = frame_alloc();
            if (table_phys == 0u) {
                return 0;
            }

            table = paging_table_ptr(table_phys);
            for (uint32_t i = 0; i < PAGE_ENTRIES; i++) {
                table[i] = 0u;
            }

            paging_directory[directory_index] = table_phys | PAGE_PRESENT | PAGE_WRITABLE;
        } else {
            table = paging_table_ptr(table_phys);
        }

        table[table_index] = addr | (flags | PAGE_PRESENT);
    }

    return 1;
}

int paging_is_ready(void) {
    return paging_ready;
}

uint32_t paging_directory_addr(void) {
    if (!paging_ready || paging_directory == NULL) {
        return 0u;
    }

    return (uint32_t)(uintptr_t)paging_directory;
}
