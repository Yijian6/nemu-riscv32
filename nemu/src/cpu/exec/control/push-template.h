#include "cpu/exec/template-start.h"

#define instr push

static void do_execute() {
	/* 压栈两步走：先给栈腾出 4 字节空间（栈往低地址增长，所以是减），
	 * 再把值写进新的栈顶。不管源操作数名义上多宽，压进栈的永远是 4 字节。 */
	cpu.esp -= 4;
	swaddr_write(cpu.esp, 4, op_src->val);
	print_asm_template1();
}

#if DATA_BYTE == 4
make_instr_helper(r)     /* PUSH r32      : 0x50-0x57 */
make_instr_helper(rm)    /* PUSH r/m32    : 0xff /6，压入内存里的值 */
make_instr_helper(i)     /* PUSH imm32    : 0x68 */
#endif
#if DATA_BYTE == 1
make_instr_helper(si)    /* PUSH imm8     : 0x6a，立即数要符号扩展成 32 位 */
#endif

#include "cpu/exec/template-end.h"
