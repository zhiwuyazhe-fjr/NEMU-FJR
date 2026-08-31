#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "monitor/monitor.h"
#include "nemu.h"

#include <stdio.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
	Assert(free_ != NULL, "watchpoint pool is exhausted");

	/* take one node from the head of the free list */
	WP *wp = free_;
	free_ = free_->next;

	/* insert it into the used list */
	wp->next = head;
	head = wp;

	return wp;
}

void free_wp(WP *wp) {
	Assert(wp != NULL, "free a NULL watchpoint");

	if(wp == head) {
		head = head->next;
	}
	else {
		WP *p = head;
		while(p != NULL && p->next != wp) { p = p->next; }
		Assert(p != NULL, "the watchpoint is not in use");
		p->next = wp->next;
	}

	wp->next = free_;
	free_ = wp;
}

WP* find_wp(int no) {
	WP *wp;
	for(wp = head; wp != NULL; wp = wp->next) {
		if(wp->NO == no) { return wp; }
	}
	return NULL;
}

void check_wp(void) {
	WP *wp;
	for(wp = head; wp != NULL; wp = wp->next) {
		bool success = true;
		uint32_t new_val = expr(wp->expr, &success);
		if(!success) { continue; }
		if(new_val != wp->old_val) {
			printf("\nHint watchpoint %d at address 0x%08x\n", wp->NO, cpu.eip);
			printf("old value = 0x%08x\n", wp->old_val);
			printf("new value = 0x%08x\n", new_val);
			wp->old_val = new_val;
			nemu_state = STOP;
		}
	}
}

void print_wp(void) {
	if(head == NULL) {
		printf("No watchpoints.\n");
		return;
	}
	WP *wp;
	for(wp = head; wp != NULL; wp = wp->next) {
		printf("%d\t%s\t0x%08x\n", wp->NO, wp->expr, wp->old_val);
	}
}
