#include "cpu/exec/template-start.h"

#define instr pop

static void do_execute() {
	DATA_TYPE val = swaddr_read(reg_l(R_ESP), DATA_BYTE, R_SS);
	reg_l(R_ESP) += DATA_BYTE;
	OPERAND_W(op_src, val);

	print_asm_template1();
}

#if DATA_BYTE == 2 || DATA_BYTE == 4
make_instr_helper(r)
make_instr_helper(rm)
#endif

#include "cpu/exec/template-end.h"
