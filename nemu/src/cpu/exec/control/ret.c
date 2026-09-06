#include "cpu/exec/helper.h"

/* RET：从栈顶弹出返回地址送入 eip。目标地址是绝对地址（不是像 jmp/call 那样的
 * 相对偏移），所以要用 jmp_rm_l 那种写法：先减去本条指令的长度(1字节)，
 * 等 cpu_exec() 事后把这个长度加回来，两次相加抵消，刚好落在真正的返回地址上。 */
make_helper(ret) {
	swaddr_t ret_addr = swaddr_read(cpu.esp, 4);
	cpu.esp += 4;
	cpu.eip = ret_addr - 1;

	print_asm("ret");
	return 1;
}
