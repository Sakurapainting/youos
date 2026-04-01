#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_SIZE 4096u
#define PAGE_ENTRIES 1024u

#define PAGE_PRESENT 0x001u
#define PAGE_WRITABLE 0x002u

void paging_init(void);
int paging_is_ready(void);
uint32_t paging_directory_addr(void);
int paging_map_identity(uint32_t start_addr, uint32_t length, uint32_t flags);

#endif
