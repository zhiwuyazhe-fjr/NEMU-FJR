#ifndef __TLB_H__
#define __TLB_H__

#include "common.h"

#define TLB_SIZE 64

typedef struct {
	bool valid;
	uint32_t tag;        /* virtual page number, i.e. lnaddr >> 12 */
	uint32_t page_frame; /* physical page number, i.e. hwaddr >> 12 */
} TLBEntry;

void init_tlb();
void tlb_flush();

/* read the TLB, return the translated physical address on hit,
 * or -1 on miss (the caller should perform a page walk then). */
int tlb_read(lnaddr_t addr, hwaddr_t *pa);

/* after a page walk, cache the translation of the page containing
 * addr into a randomly selected entry. */
void tlb_fill(lnaddr_t addr, hwaddr_t pa);

#endif
