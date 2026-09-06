#include "cpu/exec/template-start.h"

#define instr pop

static void do_execute() {
	/* 和 push 反过来：先从栈顶读出值，再把栈顶指针往上移 4 字节归还空间。 */
	DATA_TYPE val = MEM_R(cpu.esp);
	cpu.esp += 4;
	OPERAND_W(op_src, val);
	print_asm_template1();
}

make_instr_helper(r)

#include "cpu/exec/template-end.h"
