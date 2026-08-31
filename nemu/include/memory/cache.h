#ifndef __CACHE_H__
#define __CACHE_H__

#include "common.h"

/* the cache is configurable through these macros */
#define CACHE_BLOCK_SIZE     64            /* 64B block */

#define L1_CACHE_SIZE        (64 * 1024)   /* 64KB */
#define L1_ASSOCIATIVITY     8             /* 8-way set associative */
#define L1_NR_SET            (L1_CACHE_SIZE / (CACHE_BLOCK_SIZE * L1_ASSOCIATIVITY))

/* simulate the cost of memory accessing: 2 cycles on hit, 200 on miss */
extern uint64_t mem_cycles;

void init_cache();
uint32_t cache_read(hwaddr_t addr, size_t len);
void cache_write(hwaddr_t addr, size_t len, uint32_t data);

#endif
