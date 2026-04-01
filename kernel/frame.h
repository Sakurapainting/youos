#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>

void frame_init(uint32_t multiboot_magic, uint32_t multiboot_addr);
uint32_t frame_alloc(void);
void frame_free(uint32_t addr);
void frame_reserve_range(uint32_t start_addr, uint32_t length);
void frame_release_range(uint32_t start_addr, uint32_t length);
uint32_t frame_total(void);
uint32_t frame_free_count(void);
uint32_t frame_used_count(void);
int frame_is_ready(void);

#endif