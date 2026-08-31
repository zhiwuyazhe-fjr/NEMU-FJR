#include "cpu/exec/helper.h"
#include "jcc.h"

/* condition codes, cc = low 4 bits of the second opcode byte */
const char *jcc_name [16] = {
	"o", "no", "b", "nb", "e", "ne", "be", "a",
	"s", "ns", "p", "np", "l", "nl", "le", "g"
};

bool jcc_cond(uint8_t cc) {
	switch(cc) {
		case 0x0: return cpu.eflags.OF == 1;
		case 0x1: return cpu.eflags.OF == 0;
		case 0x2: return cpu.eflags.CF == 1;
		case 0x3: return cpu.eflags.CF == 0;
		case 0x4: return cpu.eflags.ZF == 1;
		case 0x5: return cpu.eflags.ZF == 0;
		case 0x6: return cpu.eflags.CF == 1 || cpu.eflags.ZF == 1;
		case 0x7: return cpu.eflags.CF == 0 && cpu.eflags.ZF == 0;
		case 0x8: return cpu.eflags.SF == 1;
		case 0x9: return cpu.eflags.SF == 0;
		case 0xa: return cpu.eflags.PF == 1;
		case 0xb: return cpu.eflags.PF == 0;
		case 0xc: return cpu.eflags.SF != cpu.eflags.OF;
		case 0xd: return cpu.eflags.SF == cpu.eflags.OF;
		case 0xe: return cpu.eflags.ZF == 1 || cpu.eflags.SF != cpu.eflags.OF;
		case 0xf: return cpu.eflags.ZF == 0 && cpu.eflags.SF == cpu.eflags.OF;
		default: return false;
	}
}

/* short jump, opcode 0x70 - 0x7f */
make_helper(jcc_si_b) {
	int len = decode_si_b(eip + 1);
	uint8_t cc = ops_decoded.opcode & 0xf;

	if(jcc_cond(cc)) {
		cpu.eip += op_src->simm;
	}
	print_asm("j%s %x", jcc_name[cc], cpu.eip + 1 + len);
	return 1 + len;
}

/* near jump, opcode 0x0f 0x80 - 0x8f */
make_helper(jcc_si_l) {
	int len = decode_si_l(eip + 1);
	uint8_t cc = ops_decoded.opcode & 0xf;

	if(jcc_cond(cc)) {
		cpu.eip += op_src->simm;
	}
	print_asm("j%s %x", jcc_name[cc], cpu.eip + 1 + len);
	return 1 + len;
}
