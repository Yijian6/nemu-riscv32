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

/* RET imm16 (0xc2)：返回之后再把 imm16 个字节的实参从栈上丢掉。
 * 用在被调用方负责清理参数的调用约定上（gcc 对返回结构体的函数会这么干，
 * struct 这个测试用例里的 ret $0x4 就是）。指令共 3 字节：操作码 + 2字节立即数。 */
make_helper(ret_i) {
	uint16_t imm = instr_fetch(eip + 1, 2);

	swaddr_t ret_addr = swaddr_read(cpu.esp, 4);
	cpu.esp += 4 + imm;
	cpu.eip = ret_addr - 3;

	print_asm("ret $0x%x", imm);
	return 3;
}
