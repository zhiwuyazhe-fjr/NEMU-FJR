#include "cpu/exec/template-start.h"

#define instr call

static void do_execute() {
	/* the return address is the instruction after the call */
	DATA_TYPE ret_addr = cpu.eip + 1 + DATA_BYTE;
	swaddr_write(reg_l(R_ESP) - 4, 4, ret_addr);
	reg_l(R_ESP) -= 4;

	cpu.eip += op_src->val;
	print_asm(str(instr) " %x", cpu.eip + 1 + DATA_BYTE);
}

make_instr_helper(si)

#if DATA_BYTE == 4
make_helper(call_rm_l) {
	int len = decode_rm_l(eip + 1);
	swaddr_write(reg_l(R_ESP) - 4, 4, cpu.eip + len + 1);
	reg_l(R_ESP) -= 4;

	cpu.eip = op_src->val - len - 1;
	print_asm(str(instr) " *%s", op_src->str);
	return len + 1;
}
#endif

#include "cpu/exec/template-end.h"
