#include "cpu/exec/template-start.h"

#define instr push

static void do_execute() {
	/* 压栈两步走：先给栈腾出 4 字节空间（栈往低地址增长，所以是减），
	 * 再把值写进新的栈顶。 */
	cpu.esp -= 4;
	swaddr_write(cpu.esp, 4, op_src->val);
	print_asm_template1();
}

make_instr_helper(r)

#include "cpu/exec/template-end.h"
