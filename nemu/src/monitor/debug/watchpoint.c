#include "nemu.h"
#include "monitor/watchpoint.h"
#include "monitor/expr.h"

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

/* ---------- PA1 task 6: watchpoint pool management ---------- */

/* Take a free node off the free_ list and append it to the in-use list.
 * Appending at the tail (rather than pushing at the head) keeps `info w'
 * listing the watchpoints in the order they were created. */
WP* new_wp() {
	/* The manual explicitly allows simply aborting when the pool runs dry. */
	Assert(free_ != NULL, "no free watchpoint left (NR_WP = %d)", NR_WP);

	WP *wp = free_;
	free_ = free_->next;

	wp->next = NULL;
	if (head == NULL) {
		head = wp;
	}
	else {
		WP *p = head;
		while (p->next != NULL) { p = p->next; }
		p->next = wp;
	}

	return wp;
}

/* Unlink wp from the in-use list and push it back onto the free_ list. */
void free_wp(WP *wp) {
	Assert(wp != NULL, "trying to free a NULL watchpoint");

	if (head == wp) {
		head = wp->next;
	}
	else {
		WP *p = head;
		while (p != NULL && p->next != wp) { p = p->next; }
		Assert(p != NULL, "watchpoint %d is not in the in-use list", wp->NO);
		p->next = wp->next;
	}

	wp->next = free_;
	free_ = wp;
}

/* ---------- PA1 task 7: watchpoint functionality ---------- */

/* Evaluate the expression once to get its initial value, then remember both
 * the text and that value. Evaluation happens BEFORE new_wp() so that a bad
 * expression doesn't consume a pool node. */
WP* set_watchpoint(char *e, bool *success) {
	uint32_t val = expr(e, success);
	if (!*success) { return NULL; }

	WP *wp = new_wp();
	strncpy(wp->expr_str, e, sizeof(wp->expr_str) - 1);
	wp->expr_str[sizeof(wp->expr_str) - 1] = '\0';
	wp->old_val = val;

	return wp;
}

bool delete_watchpoint(int no) {
	WP *p;
	for (p = head; p != NULL; p = p->next) {
		if (p->NO == no) {
			free_wp(p);
			return true;
		}
	}
	return false;
}

void list_watchpoints() {
	if (head == NULL) {
		printf("No watchpoints.\n");
		return;
	}

	printf("%-8s%-8s%-32s%s\n", "Num", "Type", "What", "Value");

	WP *p;
	for (p = head; p != NULL; p = p->next) {
		printf("%-8d%-8s%-32s0x%08x\n", p->NO, "watch", p->expr_str, p->old_val);
	}
}

/* Called after every instruction from cpu_exec(). Returns true if any watched
 * expression changed value, in which case the caller stops execution.
 * `eip' is the address of the instruction that just ran, i.e. the one that
 * caused the change. */
bool check_watchpoints(swaddr_t eip) {
	/* Fast path: evaluating expressions means running the regex lexer, which
	 * would be painfully slow to do once per instruction for nothing. */
	if (head == NULL) { return false; }

	bool triggered = false;
	WP *p;

	for (p = head; p != NULL; p = p->next) {
		bool success = true;
		uint32_t new_val = expr(p->expr_str, &success);

		if (!success) {
			printf("Watchpoint %d: cannot evaluate '%s' any more, skipping it.\n",
					p->NO, p->expr_str);
			continue;
		}

		if (new_val != p->old_val) {
			/* The first line below is the format the lab manual requires. */
			printf("\nHint watchpoint %d at address 0x%08x\n", p->NO, eip);
			printf("expr      = %s\n", p->expr_str);
			printf("old value = 0x%08x\n", p->old_val);
			printf("new value = 0x%08x\n", new_val);

			p->old_val = new_val;
			triggered = true;
		}
	}

	return triggered;
}


