#include "cpu/exec/helper.h"
#include "cpu/reg.h"
#include "cpu/mmu.h"

/* ea: JMP ptr16:32, jump to a far address given by a 6-byte immediate.
 * It is the only way to load CS: after the descriptor cache of CS is
 * reloaded from the GDT, fetching continues at the 32-bit offset. */
make_helper(ljmp) {
	uint32_t offset = instr_fetch(eip + 1, 4);
	uint16_t selector = instr_fetch(eip + 5, 2);

	load_sreg(R_CS, selector);
	cpu.eip = offset - 7;

	print_asm("ljmp $0x%x,$0x%x", selector, offset);
	return 7;
}
