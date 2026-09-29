#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "common.h"

#define HW_MEM_SIZE (128 * 1024 * 1024)

extern uint8_t *hw_mem;

/* convert the hardware address in the test program to virtual address in NEMU */
#define hwa_to_va(p) ((void *)(hw_mem + (unsigned)p))
/* convert the virtual address in NEMU to hardware address in the test program */
#define va_to_hwa(p) ((hwaddr_t)((void *)p - (void *)hw_mem))

#define hw_rw(addr, type) *(type *)({\
	Assert(addr < HW_MEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
	hwa_to_va(addr); \
})

uint32_t swaddr_read(swaddr_t, size_t, uint8_t sreg);
uint32_t lnaddr_read(lnaddr_t, size_t);
uint32_t hwaddr_read(hwaddr_t, size_t);
void swaddr_write(swaddr_t, size_t, uint32_t, uint8_t sreg);
void lnaddr_write(lnaddr_t, size_t, uint32_t);
void hwaddr_write(hwaddr_t, size_t, uint32_t);

/* Segment translation: virtual address -> linear address.
 * In the real mode (CR0.PE = 0) it is the identity mapping.
 * sreg is the encoding of the segment register to use. */
lnaddr_t seg_translate(swaddr_t addr, size_t len, uint8_t sreg);

/* Page translation: linear address -> physical address, performed by a
 * page walk starting from CR3 (with the TLB in front of it).  Only used
 * when both CR0.PE and CR0.PG are set.  addr must not cross a page
 * boundary; lnaddr_read()/lnaddr_write() split such accesses before
 * calling this. */
hwaddr_t page_translate(lnaddr_t addr);

/* The query version used by the `page' command in the monitor:
 * on success stores the physical address and returns 0, otherwise
 * returns a nonzero value without terminating NEMU. */
int page_translate_query(lnaddr_t addr, hwaddr_t *pa);

#endif
