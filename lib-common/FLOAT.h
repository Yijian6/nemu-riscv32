#ifndef __FLOAT_H__
#define __FLOAT_H__

#include "trap.h"

typedef int FLOAT;

/* 约定：实数 a 的 FLOAT 表示是 A = a * 2^16（小数部分截断）。
 * 于是"乘 2^16"就是左移 16 位，"除 2^16"就是右移 16 位。 */

static inline int F2int(FLOAT a) {
	return a >> 16;
}

static inline FLOAT int2F(int a) {
	return a << 16;
}

/* b 是普通整数(没有乘过 2^16)，所以 (a*b) * 2^16 = A * b，
 * 不需要先把 b 转成 FLOAT 再走 F_mul_F 那条 64 位的慢路。 */
static inline FLOAT F_mul_int(FLOAT a, int b) {
	return a * b;
}

/* 同理 (a/b) * 2^16 = A / b。 */
static inline FLOAT F_div_int(FLOAT a, int b) {
	return a / b;
}

FLOAT f2F(float);
FLOAT F_mul_F(FLOAT, FLOAT);
FLOAT F_div_F(FLOAT, FLOAT);
FLOAT Fabs(FLOAT);
FLOAT sqrt(FLOAT);
FLOAT pow(FLOAT, FLOAT);

// used when calling printf/sprintf to format a FLOAT argument
#define FLOAT_ARG(f) (long long)f

void init_FLOAT_vfprintf(void);

#endif
