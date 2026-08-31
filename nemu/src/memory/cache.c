#include "common.h"
#include <stdlib.h>
#include "memory/cache.h"
#include "memory/memory.h"
#include "burst.h"
#include "misc.h"

/* Simulate an L1 cache:
 *   64B block, 64KB in total, 8-way set associative, random
 *   replacement, write through, not write allocate.
 */

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

uint64_t mem_cycles = 0;

typedef struct {
	uint8_t block[CACHE_BLOCK_SIZE];
	uint32_t tag;
	bool valid;
} L1_cache_line;

static L1_cache_line l1_cache[L1_NR_SET][L1_ASSOCIATIVITY];

void init_cache() {
	int i, j;
	for(i = 0; i < L1_NR_SET; i ++) {
		for(j = 0; j < L1_ASSOCIATIVITY; j ++) {
			l1_cache[i][j].valid = false;
		}
	}
}

#define CACHE_MASK (CACHE_BLOCK_SIZE - 1)

static void l1_fill_block(hwaddr_t addr, void *buf) {
	/* bring the block which contains addr into the cache,
	 * and copy the whole block out */
	uint32_t block_nr = addr / CACHE_BLOCK_SIZE;
	uint32_t set = block_nr % L1_NR_SET;
	uint32_t tag = block_nr / L1_NR_SET;

	int i, way = -1;
	for(i = 0; i < L1_ASSOCIATIVITY; i ++) {
		if(l1_cache[set][i].valid && l1_cache[set][i].tag == tag) {
			way = i;
			break;
		}
	}

	if(way < 0) {
		/* miss: pick a victim randomly and load the block */
		way = rand() % L1_ASSOCIATIVITY;
		uint32_t block_addr = addr & ~CACHE_MASK;
		int k;
		for(k = 0; k < CACHE_BLOCK_SIZE / 4; k ++) {
			*(uint32_t *)(l1_cache[set][way].block + k * 4)
				= dram_read(block_addr + k * 4, 4);
		}
		l1_cache[set][way].valid = true;
		l1_cache[set][way].tag = tag;

		mem_cycles += 200;
	}
	else {
		mem_cycles += 2;
	}

	memcpy(buf, l1_cache[set][way].block, CACHE_BLOCK_SIZE);
}

uint32_t cache_read(hwaddr_t addr, size_t len) {
	Assert(addr < HW_MEM_SIZE, "physical address %x is outside of the physical memory!", addr);
	assert(len == 1 || len == 2 || len == 4);

	uint32_t offset = addr & CACHE_MASK;
	uint8_t temp[2 * CACHE_BLOCK_SIZE];

	/* read the block which contains addr */
	l1_fill_block(addr, temp);

	if(offset + len > CACHE_BLOCK_SIZE) {
		/* data crosses the block boundary */
		l1_fill_block(addr + CACHE_BLOCK_SIZE, temp + CACHE_BLOCK_SIZE);
	}

	return unalign_rw(temp + offset, 4) & (~0u >> ((4 - len) << 3));
}

static void l1_write_block(hwaddr_t addr, uint8_t *bytes, size_t len) {
	/* write through: the data always goes to the DRAM */
	uint32_t data;
	memcpy(&data, bytes, 4);
	dram_write(addr, len, data);

	/* update the cached copy if the block is already in the cache */
	uint32_t block_nr = addr / CACHE_BLOCK_SIZE;
	uint32_t set = block_nr % L1_NR_SET;
	uint32_t tag = block_nr / L1_NR_SET;

	int i;
	for(i = 0; i < L1_ASSOCIATIVITY; i ++) {
		if(l1_cache[set][i].valid && l1_cache[set][i].tag == tag) {
			memcpy(l1_cache[set][i].block + (addr & CACHE_MASK), bytes, len);
			break;
		}
	}
	/* not write allocate: nothing to do on a miss */
}

void cache_write(hwaddr_t addr, size_t len, uint32_t data) {
	Assert(addr < HW_MEM_SIZE, "physical address %x is outside of the physical memory!", addr);
	assert(len == 1 || len == 2 || len == 4);

	uint8_t bytes[8];
	memcpy(bytes, &data, 4);

	uint32_t offset = addr & CACHE_MASK;
	size_t done = 0;

	while(done < len) {
		size_t chunk = len - done;
		if(offset + chunk > CACHE_BLOCK_SIZE) {
			chunk = CACHE_BLOCK_SIZE - offset;
		}
		l1_write_block(addr + done, bytes + done, chunk);
		done += chunk;
		offset = 0;
	}
}
