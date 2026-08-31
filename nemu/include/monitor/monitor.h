#ifndef __MONITOR_H__
#define __MONITOR_H__

#include "common.h"

enum { STOP, RUNNING, END };
extern int nemu_state;

uint32_t lookup_sym(const char *name, int want_func, bool *success);
void func_name(uint32_t addr, char *buf);

#endif
