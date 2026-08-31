#include <stdio.h>
#include <stdint.h>
#include "FLOAT.h"

extern char _vfprintf_internal;
extern char _ppfs_setargs;
extern char _fpmaxtostr;
extern int __stdio_fwrite(char *buf, int len, FILE *stream);

#ifdef RUN_ON_LINUX
#include <sys/mman.h>
/* code segments are read only on GNU/Linux, make the pages writable
 * before patching; NEMU has no such protection, so this is not needed
 * (and not possible) there */
static void unprotect(char *p, int len) {
	uintptr_t page = (uintptr_t)p & ~0xfff;
	mprotect((void *)page, len + (p - (char *)page), PROT_READ | PROT_WRITE | PROT_EXEC);
}
#else
static void unprotect(char *p, int len) { }
#endif

/* format a FLOAT argument `f' and write the formating result to
 * `stream', keeping 6 fractional digits by truncating */
__attribute__((used)) static int format_FLOAT(FILE *stream, FLOAT f) {
	char buf[80];
	char *p = buf;
	uint32_t a = (uint32_t)f;

	if((int32_t)a < 0) {
		*p ++ = '-';
		a = -(int32_t)a;
	}

	/* integer part */
	uint32_t ip = a >> 16;
	char ipbuf[16];
	int n = 0;
	do {
		ipbuf[n ++] = '0' + ip % 10;
		ip /= 10;
	} while(ip != 0);
	while(n != 0) {
		*p ++ = ipbuf[-- n];
	}

	*p ++ = '.';

	/* fractional part: (a & 0xffff) / 2^16, 6 digits by truncating */
	uint32_t frac = ((uint64_t)(a & 0xffff) * 1000000) >> 16;
	int i;
	for(i = 5; i >= 0; i --) {
		p[i] = '0' + frac % 10;
		frac /= 10;
	}
	p += 6;

	return __stdio_fwrite(buf, p - buf, stream);
}

static void modify_vfprintf() {
	/* offsets of the patched instructions relative to the beginnings of
	 * the corresponding functions; they only depend on the code inside
	 * uclibc's vfprintf.o, so they are the same for every user program */
	const int o_fld = 0x2e4;        /* the `fldl/fldt' that loads the double */
	const int o_merge = 0x2ea;     /* where the two load paths merge */
	const int o_push_stream = 0x2ff;/* the `pushl' that pushes the stream */
	const int o_call = 0x306;       /* the `call _fpmaxtostr' */
	const int o_add = 0x30b;        /* the `add $0x20,%esp' after the call */

	uint8_t *base = (uint8_t *)&_vfprintf_internal;
	uint8_t *p;
	int i;

	unprotect((char *)base + o_fld, 0x400);

	/* clear the floating point loads */
	for(i = o_fld; i < o_merge; i ++) {
		base[i] = 0x90;             /* nop */
	}

	/* instead of pushing the converted double, push the FLOAT itself;
	 * this is where both load paths merge, so it always executes */
	p = base + o_merge;
	p[0] = 0xff;                    /* pushl (%edx) */
	p[1] = 0x32;

	/* clear the argument preparing instructions in between */
	for(i = o_merge + 2; i < o_push_stream; i ++) {
		base[i] = 0x90;             /* nop */
	}

	/* push the stream; %esp is 0x18 higher than the original code here,
	 * so the offset of the stream becomes 0x18c - 0x18 = 0x174 */
	p = base + o_push_stream;
	p[0] = 0xff;                    /* pushl 0x1a4(%esp) */
	p[1] = 0xb4;
	p[2] = 0x24;
	*(uint32_t *)(p + 3) = 0x174;

	/* call format_FLOAT instead of _fpmaxtostr */
	p = base + o_call;
	*(uint32_t *)(p + 1) = (uint32_t)((uint8_t *)format_FLOAT - (p + 5));

	/* only 2 words are pushed now, cleanup 8 bytes instead of 0x20 */
	p = base + o_add;
	p[0] = 0x83;                    /* add $0x8,%esp */
	p[1] = 0xc4;
	p[2] = 0x08;
}

static void modify_ppfs_setargs() {
	const int o_jmp = 0x71;         /* right after `cmp $0x7' (PA_DOUBLE) */
	uint8_t *base = (uint8_t *)&_ppfs_setargs;

	unprotect((char *)base + o_jmp, 0x100);

	/* let the PA_DOUBLE branch jump to the code that fetches a
	 * 64-bit integer argument (the PA_INT|PA_FLAG_LONG_LONG branch),
	 * so that no floating point instructions are executed */
	base[o_jmp] = 0xeb;             /* jmp +0x30 */
	base[o_jmp + 1] = 0x30;

	int i;
	for(i = o_jmp + 2; i < o_jmp + 10; i ++) {
		base[i] = 0x90;             /* nop out the fldl/fstpl */
	}
}

void init_FLOAT_vfprintf() {
	modify_vfprintf();
	modify_ppfs_setargs();
}
