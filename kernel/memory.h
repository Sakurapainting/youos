#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

void memory_init(uint32_t multiboot_magic, uint32_t multiboot_addr);
uint32_t memory_total_kb(void);
uint32_t memory_available_kb(void);
uint32_t memory_region_count(void);
int memory_is_ready(void);

#endif