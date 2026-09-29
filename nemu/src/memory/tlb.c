#include "nemu.h"
#include "cpu/mmu.h"
#include "memory/tlb.h"

#include <stdlib.h>
#include <time.h>

static TLBEntry tlb [TLB_SIZE];

void init_tlb() {
	tlb_flush();
	srand(time(0));
}

/* IA-32 flushes the whole TLB on every CR3 update, so that a stale
 * translation of another address space can never be reused. */
void tlb_flush() {
	memset(tlb, 0, sizeof(tlb));
}

int tlb_read(lnaddr_t addr, hwaddr_t *pa) {
	uint32_t vpn = addr >> 12;
	int i;
	for(i = 0; i < TLB_SIZE; i ++) {
		if(tlb[i].valid && tlb[i].tag == vpn) {
			*pa = (tlb[i].page_frame << 12) | (addr & PAGE_MASK);
			return 0;
		}
	}
	return -1;
}

void tlb_fill(lnaddr_t addr, hwaddr_t pa) {
	int victim = rand() % TLB_SIZE;
	tlb[victim].valid = true;
	tlb[victim].tag = addr >> 12;
	tlb[victim].page_frame = pa >> 12;
}
