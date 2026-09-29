#include "trap.h"

.globl _start
_start:
# Under IA32_PAGE the kernel loads the user program into its own
# address space, whose stack grows down from 0xc0000000.  Enter the
# program on that stack directly; the kernel has already pushed 16
# bytes below the stack top to keep the `bt' command happy.
	movl $0, %ebp
	movl $0xbffffff0, %esp
	call main

	HIT_GOOD_TRAP
