#include "common.h"
#include "memory/cache.h"
#include "cpu/reg.h"

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

/* Memory accessing interfaces */

uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	return cache_read(addr, len);
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	cache_write(addr, len, data);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	return hwaddr_read(addr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	hwaddr_write(addr, len, data);
}

lnaddr_t seg_translate(swaddr_t addr, size_t len, uint8_t sreg) {
	if(cpu.cr0.protect_enable) {
		/* The offset must stay inside the segment.  This is the place
		 * where a real IA-32 CPU would raise a segment fault. */
		Assert(addr <= cpu.sreg[sreg].limit && addr + len - 1 <= cpu.sreg[sreg].limit,
				"segment limit exceeded, addr = 0x%08x, len = %d, sreg = %d, limit = 0x%08x",
				addr, (int)len, sreg, cpu.sreg[sreg].limit);

		return cpu.sreg[sreg].base + addr;
	}
	return addr;
}

uint32_t swaddr_read(swaddr_t addr, size_t len, uint8_t sreg) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	return lnaddr_read(seg_translate(addr, len, sreg), len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data, uint8_t sreg) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_write(seg_translate(addr, len, sreg), len, data);
}

