#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* the expression being watched and its last value */
	char expr[64];
	uint32_t old_val;

} WP;

WP* new_wp(void);
void free_wp(WP *wp);
WP* find_wp(int no);
void check_wp(void);
void print_wp(void);

#endif
