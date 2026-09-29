#include "cpu/exec/helper.h"

make_helper(ret) {
	cpu.eip = swaddr_read(reg_l(R_ESP), 4, R_SS);
	reg_l(R_ESP) += 4;

	print_asm("ret");
	return 0;
}

make_helper(ret_i16) {
	int imm16 = instr_fetch(eip + 1, 2);
	cpu.eip = swaddr_read(reg_l(R_ESP), 4, R_SS);
	reg_l(R_ESP) += 4 + imm16;

	print_asm("ret $0x%x", imm16);
	return 0;
}
