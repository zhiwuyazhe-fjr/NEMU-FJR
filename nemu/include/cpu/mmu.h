#ifndef __NEMU_CPU_MMU_H__
#define __NEMU_CPU_MMU_H__

#include "common.h"

/* The 64-bit segment descriptor for code/data segments,
 * see the i386 manual for the meaning of each field. */
typedef struct SegmentDescriptor {
	uint32_t limit_15_0          : 16;
	uint32_t base_15_0           : 16;
	uint32_t base_23_16          : 8;
	uint32_t type                : 4;
	uint32_t segment_type        : 1;
	uint32_t privilege_level     : 2;
	uint32_t present             : 1;
	uint32_t limit_19_16         : 4;
	uint32_t soft_use            : 1;
	uint32_t operation_size      : 1;
	uint32_t pad0                : 1;
	uint32_t granularity         : 1;
	uint32_t base_31_24          : 8;
} SegDesc;

/* load the visible part and the descriptor cache of a segment register */
void load_sreg(uint8_t sreg_id, uint16_t sel);

#endif
