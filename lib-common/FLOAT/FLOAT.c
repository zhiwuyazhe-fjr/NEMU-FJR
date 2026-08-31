#include <stdint.h>
#include "FLOAT.h"

FLOAT F_mul_F(FLOAT a, FLOAT b) {
	return ((int64_t)a * b) >> 16;
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
	/* Dividing two 64-bit integers needs the support of `libgcc'.
	 * A "64/32" division is enough here: put (a << 16) into edx:eax
	 * and execute `idivl' to divide it by b. */
	int q, r;
	int64_t dividend = (int64_t)a << 16;
	asm volatile("idivl %4"
			: "=a"(q), "=d"(r)
			: "a"((int)(dividend & 0xffffffff)),
			  "d"((int)((uint64_t)dividend >> 32)),
			  "rm"(b));
	return q;
}

FLOAT f2F(float a) {
	/* retrieve the bit representation of `a' without performing
	 * any floating point arithmetic on it */
	union { float f; int i; } u;
	u.f = a;

	int sign = u.i >> 31;
	int exp = (u.i >> 23) & 0xff;
	int mant = u.i & 0x7fffff;

	if(exp == 0) {
		/* zero or too small, just round to 0 */
		return 0;
	}

	mant |= 0x800000;
	/* FLOAT = mant * 2^(exp - 127 - 23 + 16) */
	int shift = exp - 134;
	FLOAT ret;
	if(shift >= 0) {
		ret = mant << shift;
	}
	else {
		ret = mant >> (-shift);
	}
	return sign ? -ret : ret;
}

FLOAT Fabs(FLOAT a) {
	return a < 0 ? -a : a;
}

/* Functions below are already implemented */

FLOAT sqrt(FLOAT x) {
	FLOAT dt, t = int2F(2);

	do {
		dt = F_div_int((F_div_F(x, t) - t), 2);
		t += dt;
	} while(Fabs(dt) > f2F(1e-4));

	return t;
}

FLOAT pow(FLOAT x, FLOAT y) {
	/* we only compute x^0.333 */
	FLOAT t2, dt, t = int2F(2);

	do {
		t2 = F_mul_F(t, t);
		dt = (F_div_F(x, t2) - t) / 3;
		t += dt;
	} while(Fabs(dt) > f2F(1e-4));

	return t;
}
