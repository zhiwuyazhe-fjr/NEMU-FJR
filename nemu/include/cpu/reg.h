#ifndef __REG_H__
#define __REG_H__

#include "common.h"

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI };
enum { R_AL, R_CL, R_DL, R_BL, R_AH, R_CH, R_DH, R_BH };

/* The encoding of segment registers in i386: ES=0, CS=1, SS=2, DS=3. */
enum { R_ES, R_CS, R_SS, R_DS, NR_SREG };

/* A segment register consists of a visible part (the selector) and
 * an invisible part (the descriptor cache).  NEMU only caches the
 * base and the limit, which are everything segmentation needs. */
typedef struct {
	uint16_t sel;
	uint32_t base;
	uint32_t limit;
} SREG;

/* The Global Descriptor Table Register.  The base it holds is a
 * linear address, which must not be translated again. */
typedef struct {
	uint32_t base;
	uint16_t limit;
} GDTR;

/* The Control Register 0. */
typedef union CR0 {
	struct {
		uint32_t protect_enable      : 1;
		uint32_t pad0                : 19;
		uint32_t no_write_through    : 1;
		uint32_t cache_disable       : 1;
		uint32_t paging              : 1;
	};
	uint32_t val;
} CR0;

/* The Control Register 3, holding the physical address of the
 * current page directory. */
typedef union CR3 {
	struct {
		uint32_t pad0                : 3;
		uint32_t page_write_through  : 1;
		uint32_t page_cache_disable  : 1;
		uint32_t pad1                : 7;
		uint32_t page_directory_base : 20;
	};
	uint32_t val;
} CR3;

/* The order of gpr[] must match the register encoding of i386:
 * gpr[0] is EAX, gpr[1] is ECX, and so on.  The named struct
 * overlaps with gpr[] through the union, so cpu.eax is exactly
 * cpu.gpr[R_EAX]._32. */

typedef struct {
	union {
		union {
			uint32_t _32;
			uint16_t _16;
			uint8_t _8[2];
		} gpr[8];

		struct {
			uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
		};
	};

	/* Do NOT change the order of the GPRs' definitions. */

	swaddr_t eip;

	union {
		struct {
			uint32_t CF		:1;
			uint32_t pad0	:1;
			uint32_t PF		:1;
			uint32_t pad1	:1;
			uint32_t AF		:1;
			uint32_t pad2	:1;
			uint32_t ZF		:1;
			uint32_t SF		:1;
			uint32_t TF		:1;
			uint32_t IF		:1;
			uint32_t DF		:1;
			uint32_t OF		:1;
			uint32_t IOPL	:2;
			uint32_t NT		:1;
			uint32_t pad3	:1;
			uint16_t pad4;
		};
		uint32_t val;
	} eflags;

	CR0 cr0;
	CR3 cr3;
	GDTR gdtr;

	/* CS, DS, ES, SS, indexed by the i386 segment register encoding. */
	SREG sreg[NR_SREG];

} CPU_state;

extern CPU_state cpu;

static inline int check_reg_index(int index) {
	assert(index >= 0 && index < 8);
	return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._32)
#define reg_w(index) (cpu.gpr[check_reg_index(index)]._16)
#define reg_b(index) (cpu.gpr[check_reg_index(index) & 0x3]._8[index >> 2])

extern const char* regsl[];
extern const char* regsw[];
extern const char* regsb[];

#endif
