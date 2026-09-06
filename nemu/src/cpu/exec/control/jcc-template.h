#include "cpu/exec/template-start.h"

#define instr jcc

/* jcc 是 16 条指令(0x70-0x7f 短跳, 0x0f80-0x0f8f 近跳)共用一套逻辑，
 * 区别只在"低4位选哪个条件码"——这个信息 opcode 字节里已经有了
 * (ops_decoded.opcode，_2byte_esc 会把它设成 0x100|第二字节)，
 * 不用像 make_group 那样为每个子操作码建一个函数指针数组。 */
static void do_execute() {
	uint8_t cc = ops_decoded.opcode & 0xf;

	/* 分支目标是编译期就定死的，不管这次是不是真跳转都算出来打印，只用于
	 * 反汇编打印：短跳(0x70-0x7f)操作码只占1字节，近跳(0x0f 0x80-0x8f)
	 * 多一个 0x0f 前缀字节，ops_decoded.opcode 是否 > 0xff 正好能区分
	 * 两种情况(_2byte_esc 把它标成 0x100|第二字节)。 */
	int prefix_len = (ops_decoded.opcode > 0xff) ? 2 : 1;
	swaddr_t target = cpu.eip + prefix_len + DATA_BYTE + op_src->val;
	if (check_cc(cc)) {
		cpu.eip += op_src->val;
	}
	print_asm("j%s %x", cc_name[cc], target);
}

make_instr_helper(si)

#include "cpu/exec/template-end.h"
