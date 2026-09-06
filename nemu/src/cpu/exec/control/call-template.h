#include "cpu/exec/template-start.h"

#define instr call

static void do_execute() {
	/* 返回地址 = call 指令自身地址 + 整条指令长度(1字节操作码 + DATA_BYTE字节偏移量)。
	 * 此刻 cpu.eip 还是 call 指令自己的地址（还没被 cpu_exec() 更新），
	 * 所以这里手动补上 "1 + DATA_BYTE"，得到"call 之后那条指令"的地址。 */
	swaddr_t ret_addr = cpu.eip + 1 + DATA_BYTE;

	/* 压栈：先把栈顶指针往下移 4 字节，腾出空间，再把返回地址写进去。
	 * 栈上的地址永远是 32 位，所以固定用 4，不用 DATA_BYTE。 */
	cpu.esp -= 4;
	swaddr_write(cpu.esp, 4, ret_addr);

	/* 和 jmp 完全一样的相对跳转：此刻算出来的 eip 会"差一截"，
	 * 等 cpu_exec() 事后再加上指令长度，两次相加顺序不影响最终结果。 */
	cpu.eip += op_src->val;

	print_asm(str(instr) " %x", cpu.eip + 1 + DATA_BYTE);
}

make_instr_helper(si)

#include "cpu/exec/template-end.h"
