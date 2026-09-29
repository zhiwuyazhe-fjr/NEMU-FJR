#include "common.h"
#include "memory/cache.h"
#include "memory/tlb.h"
#include "cpu/reg.h"
#include "cpu/mmu.h"

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

/* Memory accessing interfaces */

uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	return cache_read(addr, len);
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	cache_write(addr, len, data);
}

/* Walk the two-level page tables rooted at CR3.  The page directory
 * base in CR3 and the page frames in the entries are all physical
 * addresses, so the tables themselves are read through hwaddr_read(),
 * never translated again. */
int page_translate_query(lnaddr_t addr, hwaddr_t *pa) {
	PDE pde;
	pde.val = hwaddr_read((cpu.cr3.page_directory_base << 12)
			| ((addr >> 22) << 2), 4);
	if(!pde.present) { return 1; }

	PTE pte;
	pte.val = hwaddr_read((pde.page_frame << 12)
			| (((addr >> 12) & 0x3ff) << 2), 4);
	if(!pte.present) { return 2; }

	*pa = (pte.page_frame << 12) | (addr & PAGE_MASK);
	return 0;
}

hwaddr_t page_translate(lnaddr_t addr) {
	hwaddr_t pa;

	/* Most translations hit in the TLB and skip the page walk. */
	if(tlb_read(addr, &pa) == 0) { return pa; }

	/* NEMU does not implement the protection mechanism, so an invalid
	 * entry always means an implementation error somewhere: terminate
	 * immediately, otherwise debugging would be extremely hard. */
	int ret = page_translate_query(addr, &pa);
	Assert(ret == 0, "page walk failed for linear address 0x%08x "
			"(present bit of the %s is 0)", addr,
			ret == 1 ? "page directory entry" : "page table entry");

	tlb_fill(addr, pa);
	return pa;
}

static int paging_enabled() {
	return cpu.cr0.protect_enable && cpu.cr0.paging;
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	if(paging_enabled()) {
		/* A datum may cross a page boundary; the two pages can map to
		 * non-contiguous physical pages, so translate them apart. */
		if((addr & ~PAGE_MASK) != ((addr + len - 1) & ~PAGE_MASK)) {
			/* cross page */
			size_t len1 = PAGE_SIZE - (addr & PAGE_MASK);
			uint32_t lo = hwaddr_read(page_translate(addr), len1);
			uint32_t hi = hwaddr_read(page_translate(addr + len1), len - len1);
			return (hi << (len1 << 3)) | lo;
		}
		return hwaddr_read(page_translate(addr), len);
	}
	return hwaddr_read(addr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	if(paging_enabled()) {
		if((addr & ~PAGE_MASK) != ((addr + len - 1) & ~PAGE_MASK)) {
			/* cross page */
			size_t len1 = PAGE_SIZE - (addr & PAGE_MASK);
			hwaddr_write(page_translate(addr), len1, data & ((1 << (len1 << 3)) - 1));
			hwaddr_write(page_translate(addr + len1), len - len1, data >> (len1 << 3));
			return;
		}
		hwaddr_write(page_translate(addr), len, data);
		return;
	}
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

