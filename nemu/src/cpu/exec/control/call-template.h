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

#if DATA_BYTE == 4
/* CALL r/m32 (0xff /2)：间接调用，跳转目标是操作数里存的一个绝对地址
 * （典型来源是函数指针，比如 integral.c 里的 fun(a) 调用）。
 * 因为是绝对地址而不是相对偏移量，不能用上面那套"事后再加长度"的加法技巧，
 * 要像 jmp_rm_l 那样先减掉本条指令的长度，让 cpu_exec() 的加法刚好抵消。 */
make_helper(call_rm_l) {
	int len = decode_rm_l(eip + 1);
	swaddr_t ret_addr = cpu.eip + 1 + len;

	cpu.esp -= 4;
	swaddr_write(cpu.esp, 4, ret_addr);

	cpu.eip = op_src->val - (len + 1);

	print_asm("call *%s", op_src->str);
	return len + 1;
}
#endif

#include "cpu/exec/template-end.h"
