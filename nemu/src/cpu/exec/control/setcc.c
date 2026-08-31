#include "cpu/exec/helper.h"
#include "jcc.h"

/* setcc, opcode 0x0f 0x90 - 0x9f */
make_helper(setcc_rm_b) {
	int len = decode_rm_b(eip + 1);
	uint8_t cc = ops_decoded.opcode & 0xf;

	write_operand_b(op_src, jcc_cond(cc) ? 1 : 0);
	print_asm("set%s %s", jcc_name[cc], op_src->str);
	return 1 + len;
}
