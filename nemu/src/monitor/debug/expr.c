#include "nemu.h"
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, NUM

	/* TODO: Add more token types (PA1 stage 2 task 5: hex numbers, register
	 * names, !=, &&, ||, !, pointer dereference) */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",		NOTYPE},			// spaces
	{"\\+",		'+'},				// plus
	{"-",		'-'},				// minus
	{"\\*",		'*'},				// multiply
	{"/",		'/'},				// divide
	{"\\(",		'('},				// left parenthesis
	{"\\)",		')'},				// right parenthesis
	{"[0-9]+",	NUM},				// decimal number
	{"==", EQ}						// equal
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE:
						/* whitespace carries no meaning, discard it */
						break;
					default: {
						if (nr_token >= sizeof(tokens) / sizeof(tokens[0])) {
							panic("too many tokens in expression");
						}
						/* Guard against overflowing the fixed-size str
						 * buffer instead of silently corrupting memory. */
						int copy_len = substr_len;
						if (copy_len >= sizeof(tokens[0].str)) {
							copy_len = sizeof(tokens[0].str) - 1;
						}
						tokens[nr_token].type = rules[i].token_type;
						memcpy(tokens[nr_token].str, substr_start, copy_len);
						tokens[nr_token].str[copy_len] = '\0';
						nr_token ++;
						break;
					}
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

/* Returns true iff tokens[p..q] is a single expression fully wrapped by
 * one matching pair of parentheses, e.g. "(2 - 1)" or "(4 + 3 * (2 - 1))",
 * but NOT "(4 + 3) * (2 - 1)" (leftmost '(' closes before reaching q) and
 * NOT "4 + 3 * (2 - 1)" (doesn't start with '(' at all). */
static bool check_parentheses(int p, int q) {
	if (tokens[p].type != '(' || tokens[q].type != ')') {
		return false;
	}

	int balance = 0;
	int i;
	for (i = p; i <= q; i ++) {
		if (tokens[i].type == '(') {
			balance ++;
		}
		else if (tokens[i].type == ')') {
			balance --;
			if (balance < 0) {
				/* a ')' with no matching '(' before it: bad expression */
				return false;
			}
			if (balance == 0 && i < q) {
				/* the '(' at p already closed before reaching q, so the
				 * whole range is NOT surrounded by ONE matching pair */
				return false;
			}
		}
	}
	return balance == 0;
}

/* Lower return value = lower precedence = more likely to be the dominant
 * operator (the last operation performed when evaluating by hand).
 * Returns -1 for tokens that aren't an operator we split on here. */
static int op_precedence(int type) {
	switch (type) {
		case '+': case '-': return 1;
		case '*': case '/': return 2;
		default: return -1;
	}
}

/* Scan tokens[p..q] (skipping anything inside a nested pair of parentheses)
 * and return the index of the dominant operator: the one with the lowest
 * precedence, breaking ties by picking the rightmost (so left-associativity,
 * e.g. "1 + 2 + 3", is handled correctly). Returns -1 if none is found. */
static int find_dominant_op(int p, int q) {
	int op = -1;
	int min_pre = 1 << 30;
	int balance = 0;
	int i;

	for (i = p; i <= q; i ++) {
		int type = tokens[i].type;
		if (type == '(') { balance ++; continue; }
		if (type == ')') { balance --; continue; }
		if (balance != 0) { continue; }

		int pre = op_precedence(type);
		if (pre == -1) { continue; }

		if (pre <= min_pre) {
			min_pre = pre;
			op = i;
		}
	}
	return op;
}

static uint32_t eval(int p, int q, bool *success) {
	if (p > q) {
		/* Bad expression, e.g. empty parentheses "()" recursing inward. */
		*success = false;
		return 0;
	}
	else if (p == q) {
		/* Single token: for now (PA1 task 3/4) this must be a number.
		 * Register names / hex numbers are added in task 5. */
		if (tokens[p].type != NUM) {
			*success = false;
			return 0;
		}
		return strtoul(tokens[p].str, NULL, 10);
	}
	else if (check_parentheses(p, q)) {
		/* Surrounded by one matching pair of parentheses: strip them. */
		return eval(p + 1, q - 1, success);
	}
	else {
		int op = find_dominant_op(p, q);
		if (op == -1) {
			*success = false;
			return 0;
		}

		uint32_t val1 = eval(p, op - 1, success);
		if (!*success) { return 0; }
		uint32_t val2 = eval(op + 1, q, success);
		if (!*success) { return 0; }

		switch (tokens[op].type) {
			case '+': return val1 + val2;
			case '-': return val1 - val2;
			case '*': return val1 * val2;
			case '/':
				if (val2 == 0) {
					*success = false;
					return 0;
				}
				return val1 / val2;
			default:
				*success = false;
				return 0;
		}
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	*success = true;
	return eval(0, nr_token - 1, success);
}

