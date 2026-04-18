#ifndef PMM_H
#define PMM_H

#include <stdint.h>

void pmm_init(uint32_t mem_size);
void* pmm_alloc_page();
void pmm_free_page(void* addr);

#endif
