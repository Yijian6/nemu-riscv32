#include "cpu/exec/template-start.h"

#define instr setcc

/* SETcc r/m8 (0x0f 0x90-0x9f)：条件成立就把目标字节置 1，否则置 0。
 * 和 jcc 用的是同一张条件码表(opcode 低4位)，只是"跳转"换成了"写一个字节"。
 * C 里的 a == b、a < b 这类布尔表达式，编译器就是用它算出 0/1 的。 */
static void do_execute() {
	uint8_t cc = ops_decoded.opcode & 0xf;

	OPERAND_W(op_src, check_cc(cc) ? 1 : 0);

	print_asm("set%s %s", cc_name[cc], op_src->str);
}

make_instr_helper(rm)

#include "cpu/exec/template-end.h"
