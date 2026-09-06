#include "FLOAT.h"

FLOAT F_mul_F(FLOAT a, FLOAT b) {
	/* A * B = (a*2^16) * (b*2^16) = (a*b) * 2^32，比我们想要的
	 * (a*b)*2^16 多乘了一个 2^16，所以结果要再右移 16 位。
	 * 中间结果 (a*b)*2^32 放不进 32 位，必须用 64 位临时变量。 */
	return ((long long)a * b) >> 16;
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
	/* Dividing two 64-bit integers needs the support of another library
	 * `libgcc', other than newlib. It is a dirty work to port `libgcc'
	 * to NEMU. In fact, it is unnecessary to perform a "64/64" division
	 * here. A "64/32" division is enough.
	 *
	 * To perform a "64/32" division, you can use the x86 instruction
	 * `div' or `idiv' by inline assembly. We provide a template for you
	 * to prevent you from uncessary details.
	 *
	 *     asm volatile ("??? %2" : "=a"(???), "=d"(???) : "r"(???), "a"(???), "d"(???));
	 *
	 * If you want to use the template above, you should fill the "???"
	 * correctly. For more information, please read the i386 manual for
	 * division instructions, and search the Internet about "inline assembly".
	 * It is OK not to use the template above, but you should figure
	 * out another way to perform the division.
	 */

	/* A / B = (a*2^16) / (b*2^16) = a/b，2^16 被约掉了，比想要的
	 * (a/b)*2^16 少了一个 2^16，所以先把被除数左移 16 位再除。
	 * 被除数是 64 位、除数是 32 位，正好对上 x86 的 idivl：
	 * 它拿 EDX:EAX 这对寄存器当 64 位被除数，除以一个 32 位操作数，
	 * 商放回 EAX、余数放回 EDX。C 里直接写 64/64 除法会去调用
	 * libgcc 的 __divdi3，而 NEMU 的用户程序没有链接 libgcc。 */
	long long dividend = (long long)a << 16;
	int quotient, remainder;

	asm volatile ("idivl %2"
			: "=a"(quotient), "=d"(remainder)
			: "r"(b), "a"((int)dividend), "d"((int)(dividend >> 32)));

	return quotient;
}

FLOAT f2F(float a) {
	/* You should figure out how to convert `a' into FLOAT without
	 * introducing x87 floating point instructions. Else you can
	 * not run this code in NEMU before implementing x87 floating
	 * point instructions, which is contrary to our expectation.
	 *
	 * Hint: The bit representation of `a' is already on the
	 * stack. How do you retrieve it to another variable without
	 * performing arithmetic operations on it directly?
	 */

	/* 关键：全程不碰浮点运算，只把 a 的那 32 个二进制位当整数读出来，
	 * 然后按 IEEE-754 单精度的格式用整数算术手工解码：
	 *   第31位 = 符号, 第30~23位 = 阶码(偏移量127), 第22~0位 = 尾数
	 *   数值 = (-1)^符号 * 1.尾数 * 2^(阶码-127)
	 * 把隐含的那个整数位 1 补回来后，(1<<23)|尾数 这个整数代表的是
	 * 1.尾数 * 2^23，于是
	 *   数值 * 2^16 = ((1<<23)|尾数) * 2^(阶码-127+16-23)
	 *               = ((1<<23)|尾数) * 2^(阶码-134)
	 * 指数为正就左移，为负就右移。 */
	int bits = *(int *)&a;
	int sign = (bits >> 31) & 1;
	int exponent = (bits >> 23) & 0xff;
	int mantissa = bits & 0x7fffff;

	FLOAT result;
	if (exponent == 0) {
		/* 阶码全 0：是 0 或者非规格化数，小到 FLOAT 根本表示不了，按 0 处理 */
		result = 0;
	} else {
		int significand = mantissa | (1 << 23);
		int shift = exponent - 134;
		result = (shift >= 0 ? (significand << shift) : (significand >> -shift));
	}

	return sign ? -result : result;
}

FLOAT Fabs(FLOAT a) {
	/* FLOAT 用补码表示负数，取绝对值就是普通整数取绝对值 */
	return a < 0 ? -a : a;
}

/* Functions below are already implemented */

FLOAT sqrt(FLOAT x) {
	FLOAT dt, t = int2F(2);

	do {
		dt = F_div_int((F_div_F(x, t) - t), 2);
		t += dt;
	} while(Fabs(dt) > f2F(1e-4));

	return t;
}

FLOAT pow(FLOAT x, FLOAT y) {
	/* we only compute x^0.333 */
	FLOAT t2, dt, t = int2F(2);

	do {
		t2 = F_mul_F(t, t);
		dt = (F_div_F(x, t2) - t) / 3;
		t += dt;
	} while(Fabs(dt) > f2F(1e-4));

	return t;
}

