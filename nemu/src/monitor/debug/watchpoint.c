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

/* 从free_链表头摘一个节点挂到head上 */
WP* new_wp() {
	Assert(free_ != NULL, "watchpoint pool is exhausted");

	WP *wp = free_;
	free_ = free_->next;

	wp->next = head;
	head = wp;

	return wp;
}

/* 从head链表摘下来还给free_ */
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

/* 按编号找正在使用的监视点 */
WP* find_wp(int no) {
	WP *wp;
	for(wp = head; wp != NULL; wp = wp->next) {
		if(wp->NO == no) { return wp; }
	}
	return NULL;
}
