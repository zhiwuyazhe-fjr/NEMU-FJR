#ifndef __CACHE_H__
#define __CACHE_H__

#include "common.h"

/* the caches are configurable through these macros */
#define CACHE_BLOCK_SIZE     64            /* 64B block */

#define L1_CACHE_SIZE        (64 * 1024)   /* 64KB */
#define L1_ASSOCIATIVITY     8             /* 8-way set associative */
#define L1_NR_SET            (L1_CACHE_SIZE / (CACHE_BLOCK_SIZE * L1_ASSOCIATIVITY))

#define L2_CACHE_SIZE        (4 * 1024 * 1024)  /* 4MB */
#define L2_ASSOCIATIVITY     16                 /* 16-way set associative */
#define L2_NR_SET            (L2_CACHE_SIZE / (CACHE_BLOCK_SIZE * L2_ASSOCIATIVITY))

/* simulate the cost of memory accessing: 2 cycles on L1 hit,
 * 200 on a miss that finally reaches the DRAM */
extern uint64_t mem_cycles;

void init_cache();
uint32_t cache_read(hwaddr_t addr, size_t len);
void cache_write(hwaddr_t addr, size_t len, uint32_t data);

#endif
