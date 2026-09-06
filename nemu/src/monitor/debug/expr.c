#include "nemu.h"
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

/* 在 elf.c 里实现：按名字到符号表中查一个变量的地址 */
bool lookup_symbol_addr(const char *name, uint32_t *addr);

enum {
	NOTYPE = 256, EQ, NEQ, AND, OR, NUM, HEX, REG, SYMB,

	/* NEG and DEREF do not come out of the lexer: '-' and '*' are ambiguous
	 * (binary minus vs. negation, multiply vs. dereference) and can only be
	 * told apart once we can look at the PREVIOUS token, which is a job for
	 * the pass in expr() below. */
	NEG, DEREF

	/* Single-character tokens ('+', '-', '*', '/', '(', ')', '!') just use
	 * their own ASCII code as the type, so they need no enumerator here. */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* NOTE: rules are tried IN ORDER and the first one that matches at the
	 * current position wins -- this is NOT longest-match. So a longer token
	 * must be listed before any shorter token it starts with:
	 *   "0x.." before "[0-9]+"  (else "0x10" is cut into "0" and "x10")
	 *   "!="   before "!"       (else "!=" is cut into "!" and "=")
	 */

	{" +",					NOTYPE},	// spaces
	{"0[xX][0-9a-fA-F]+",	HEX},		// hexadecimal number (before decimal!)
	{"[0-9]+",				NUM},		// decimal number
	{"\\$[a-zA-Z]+",			REG},		// register name, e.g. $eax ('$' is a metachar)
	{"[a-zA-Z_][a-zA-Z0-9_]*",	SYMB},		// variable name, e.g. test_data (C identifier)
	{"\\+",					'+'},		// plus
	{"-",					'-'},		// minus
	{"\\*",					'*'},		// multiply
	{"/",					'/'},		// divide
	{"\\(",					'('},		// left parenthesis
	{"\\)",					')'},		// right parenthesis
	{"==",					EQ},		// equal
	{"!=",					NEQ},		// not equal (before "!"!)
	{"&&",					AND},		// logical and
	{"\\|\\|",				OR},		// logical or ('|' is a metachar)
	{"!",					'!'}		// logical not
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
		case OR:			return 1;	// lowest
		case AND:			return 2;
		case EQ: case NEQ:	return 3;
		case '+': case '-':	return 4;
		case '*': case '/':	return 5;	// highest
		/* The UNARY operators ('!', NEG, DEREF) never split an expression into
		 * a left and a right half, so they must never be picked as the
		 * dominant operator. Falling through to -1 keeps them out of the
		 * election, and eval() handles them separately. */
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

/* Look up the value of a register token such as "$eax" / "$ax" / "$al" /
 * "$eip". The three name tables (regsl/regsw/regsb) and the reg_l/reg_w/reg_b
 * access macros all live in cpu/reg.h. */
static uint32_t get_reg_value(const char *token_str, bool *success) {
	const char *name = token_str + 1;	/* skip the leading '$' */
	int i;

	if (strcmp(name, "eip") == 0) { return cpu.eip; }

	for (i = R_EAX; i <= R_EDI; i ++) {
		if (strcmp(name, regsl[i]) == 0) { return reg_l(i); }
	}
	for (i = R_AX; i <= R_DI; i ++) {
		if (strcmp(name, regsw[i]) == 0) { return reg_w(i); }
	}
	for (i = R_AL; i <= R_BH; i ++) {
		if (strcmp(name, regsb[i]) == 0) { return reg_b(i); }
	}

	*success = false;
	return 0;
}

static uint32_t eval(int p, int q, bool *success) {
	if (p > q) {
		/* Bad expression, e.g. empty parentheses "()" recursing inward. */
		*success = false;
		return 0;
	}
	else if (p == q) {
		/* Single token: the base case of the recursion. It must be something
		 * that carries a value on its own -- a number or a register. */
		switch (tokens[p].type) {
			case NUM: return strtoul(tokens[p].str, NULL, 10);
			case HEX: return strtoul(tokens[p].str, NULL, 16);
			case REG: return get_reg_value(tokens[p].str, success);
			case SYMB: {
				/* 变量名一律返回它的地址，不是它的值：表达式求值里没有类型
				 * 系统，分不出"整型变量"和"数组"，所以统一给地址，要取值就
				 * 自己写 p *x（这一点和 GDB 不同，手册里专门说明过）。 */
				uint32_t addr;
				if (lookup_symbol_addr(tokens[p].str, &addr)) { return addr; }
				printf("Unknown variable '%s'\n", tokens[p].str);
				*success = false;
				return 0;
			}
			default:
				*success = false;
				return 0;
		}
	}
	else if (check_parentheses(p, q)) {
		/* Surrounded by one matching pair of parentheses: strip them. */
		return eval(p + 1, q - 1, success);
	}
	else {
		int op = find_dominant_op(p, q);

		if (op == -1) {
			/* No binary operator at the top level. The only remaining legal
			 * forms are the prefix unary operators, e.g. "!1", "-1", "*$esp".
			 * This check MUST come after find_dominant_op, not before:
			 * "!1 + 2" is (!1) + 2 == 2, not !(1 + 2) == 0. */
			uint32_t val;

			switch (tokens[p].type) {
				case '!':
					val = eval(p + 1, q, success);
					if (!*success) { return 0; }
					return !val;

				case NEG:
					val = eval(p + 1, q, success);
					if (!*success) { return 0; }
					return -val;

				case DEREF:
					val = eval(p + 1, q, success);
					if (!*success) { return 0; }
					/* Expressions are untyped here, so a dereference always
					 * fetches one uint32_t. Bounds-check first: dram_read()
					 * asserts on a bad address, which would kill all of NEMU
					 * just because of a typo at the debugger prompt. */
					if (val > HW_MEM_SIZE - 4) {
						printf("cannot dereference 0x%08x: outside the simulated memory\n", val);
						*success = false;
						return 0;
					}
					return swaddr_read(val, 4);

				default:
					*success = false;
					return 0;
			}
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
			case EQ:  return val1 == val2;
			case NEQ: return val1 != val2;
			case AND: return val1 && val2;
			case OR:  return val1 || val2;
			default:
				*success = false;
				return 0;
		}
	}
}

/* True if the token before position i yields a value. '-' and '*' are BINARY
 * when they follow a value (a number, a register or a closing parenthesis)
 * and UNARY otherwise -- including at the very start of the expression. */
static bool follows_a_value(int i) {
	if (i == 0) { return false; }

	switch (tokens[i - 1].type) {
		case NUM: case HEX: case REG: case SYMB: case ')': return true;
		default: return false;
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* Retype the ambiguous '-' and '*' tokens before evaluating: the lexer
	 * cannot tell "1 - 1" from "1 + -1", nor "2 * 3" from "*0x100000",
	 * because that depends on context rather than on spelling. */
	int i;
	for (i = 0; i < nr_token; i ++) {
		if (follows_a_value(i)) { continue; }

		if (tokens[i].type == '-') { tokens[i].type = NEG; }
		else if (tokens[i].type == '*') { tokens[i].type = DEREF; }
	}

	*success = true;
	return eval(0, nr_token - 1, success);
}

