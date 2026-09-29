#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"
#include "cpu/reg.h"
#include "cpu/mmu.h"
#include "memory/tlb.h"

static const char *sregs [] = {"es", "cs", "ss", "ds"};

/* Load the visible part (selector) and the invisible part (descriptor
 * cache) of the segment register with index sreg_id.
 *
 * The descriptor is fetched from the GDT, whose base stored in GDTR is
 * a linear address, so it is read through lnaddr_read() directly and
 * never translated by segmentation again. */
void load_sreg(uint8_t sreg_id, uint16_t sel) {
	Assert(sreg_id < NR_SREG, "bad segment register index %#x", sreg_id);
	cpu.sreg[sreg_id].sel = sel;

	/* The TI bit must be 0: NEMU does not simulate the LDT. */
	Assert((sel & 0x4) == 0, "LDT(selector = %#x) is not supported", sel);

	uint32_t index = sel >> 3;
	lnaddr_t desc_addr = cpu.gdtr.base + index * sizeof(SegDesc);
	Assert(desc_addr + sizeof(SegDesc) - 1 <= cpu.gdtr.base + cpu.gdtr.limit,
			"segment descriptor of selector %#x is out of GDT bound", sel);

	uint8_t buf[sizeof(SegDesc)];
	int i;
	for(i = 0; i < sizeof(SegDesc); i ++) {
		buf[i] = lnaddr_read(desc_addr + i, 1);
	}
	SegDesc *desc = (void *)buf;

	/* A real CPU would raise an exception here. */
	Assert(desc->present, "segment descriptor of selector %#x is not present", sel);

	uint32_t limit = (desc->limit_19_16 << 16) | desc->limit_15_0;
	if(desc->granularity) { limit = (limit << 12) | 0xfff; }

	cpu.sreg[sreg_id].base = (desc->base_31_24 << 24) | (desc->base_23_16 << 16)
		| desc->base_15_0;
	cpu.sreg[sreg_id].limit = limit;
}

/* 0f 01 /2: LGDT m16&32, load the GDTR from memory.
 * The 2-byte limit comes first, followed by the 4-byte linear base. */
make_helper(lgdt) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);
	assert(m.mod != 3);

	Operand rm;
	int len = load_addr(eip + 1, &m, &rm);

	cpu.gdtr.limit = swaddr_read(rm.addr, 2, rm.sreg);
	cpu.gdtr.base = swaddr_read(rm.addr + 2, 4, rm.sreg);

	print_asm("lgdt 0x%x", rm.addr);
	return 1 + len;
}

/* 0f 20: MOV r/m32, CR0 */
make_helper(mov_cr2rm) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);

	Operand rm, reg;
	rm.size = 4;
	int len = read_ModR_M(eip + 1, &rm, &reg);

	uint32_t val;
	switch(m.reg) {
		case 0: val = cpu.cr0.val; break;
		case 3: val = cpu.cr3.val; break;
		default: Assert(0, "unsupported control register CR%d", m.reg);
	}

	if(m.mod == 3) { reg_l(m.R_M) = val; }
	else { swaddr_write(rm.addr, 4, val, rm.sreg); }

	print_asm("mov %s,%%cr%d", rm.str, m.reg);
	return 1 + len;
}

/* 0f 22: MOV CR0, r/m32 */
make_helper(mov_rm2cr) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);

	Operand rm, reg;
	rm.size = 4;
	int len = read_ModR_M(eip + 1, &rm, &reg);

	switch(m.reg) {
		case 0: cpu.cr0.val = rm.val; break;
		case 3:
			/* CR3 points to the page directory of the current address
			 * space; updating it must invalidate every cached
			 * translation, or a stale mapping of another address space
			 * would still be used. */
			cpu.cr3.val = rm.val;
			tlb_flush();
			break;
		default: Assert(0, "unsupported control register CR%d", m.reg);
	}

	print_asm("mov %%cr%d,%s", m.reg, rm.str);
	return 1 + len;
}

/* 8e /r: MOV Sreg, r/m16, load a segment register.
 * IA-32 forbids loading CS this way (use ljmp instead). */
make_helper(mov_rm2sreg) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);

	Operand rm, reg;
	rm.size = 2;
	int len = read_ModR_M(eip + 1, &rm, &reg);

	Assert(m.reg != R_CS, "CS cannot be loaded by mov");
	Assert(m.reg < NR_SREG, "bad segment register index %#x", m.reg);
	load_sreg(m.reg, rm.val);

	print_asm("mov %s,%%%s", rm.str, sregs[m.reg]);
	return 1 + len;
}
