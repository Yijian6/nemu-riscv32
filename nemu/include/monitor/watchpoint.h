#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* The watched expression is kept as TEXT because it must be re-evaluated
	 * after every single instruction; old_val is what it evaluated to at the
	 * previous check, which is the only way to tell that it "changed". */
	char expr_str[64];
	uint32_t old_val;

} WP;

void init_wp_pool(void);

/* Watchpoint pool management (PA1 task 6). */
WP* new_wp(void);
void free_wp(WP *wp);

/* Watchpoint functionality (PA1 task 7). */
WP* set_watchpoint(char *e, bool *success);
bool delete_watchpoint(int no);
void list_watchpoints(void);
bool check_watchpoints(swaddr_t eip);

#endif
