#ifndef __JCC_H__
#define __JCC_H__

make_helper(jcc_si_b);
make_helper(jcc_si_l);

bool jcc_cond(uint8_t cc);
extern const char *jcc_name [];

#endif
