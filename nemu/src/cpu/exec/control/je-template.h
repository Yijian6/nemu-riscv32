#include "cpu/exec/template-start.h"

#define instr je

/* JE rel8：只读 ZF，不改 EFLAGS。跳转量的加法顺序和 jmp/call 一样，
 * 靠 cpu_exec() 事后再加一次指令长度来凑出正确的绝对地址。 */
static void do_execute() {
	/* 分支目标是编译期就定死的，不管这次是不是真跳转都算出来打印，
	 * 跟 objdump 反汇编显示的地址对得上。 */
	swaddr_t target = cpu.eip + 1 + DATA_BYTE + op_src->val;
	if (cpu.eflags.ZF) {
		cpu.eip += op_src->val;
	}
	print_asm(str(instr) " %x", target);
}

make_instr_helper(si)

#include "cpu/exec/template-end.h"
