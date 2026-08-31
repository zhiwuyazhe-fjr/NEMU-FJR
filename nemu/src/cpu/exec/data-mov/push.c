#include "cpu/exec/helper.h"

#define DATA_BYTE 2
#include "push-template.h"
#undef DATA_BYTE

#define DATA_BYTE 4
#include "push-template.h"
#undef DATA_BYTE

/* for instruction encoding overloading */

make_helper_v(push_r)
make_helper_v(push_i)
make_helper_v(push_rm)

/* push sign-extended imm8, opcode 0x6a */
make_helper(push_si_v) {
	int len = decode_si_b(eip + 1);
	reg_l(R_ESP) -= 4;
	swaddr_write(reg_l(R_ESP), 4, op_src->val);

	print_asm("pushl $0x%x", op_src->val);
	return 1 + len;
}
