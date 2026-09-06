#include "cpu/exec/template-start.h"

#define instr shld

#if DATA_BYTE == 2 || DATA_BYTE == 4
/* SHLD dest, src, count：把 dest 左移 count 位，空出来的低位不补 0，
 * 而是从 src 的高位依次补进来——正好是"两个32位寄存器拼成64位再整体左移"
 * 时高半部分需要的动作。所以 gcc 编译 (long long)a << 16 会生成 shld，
 * 编译 >> 16 会生成 shrd（已有的 shrd-template.h 就是它的镜像）。 */
static void do_execute () {
	DATA_TYPE in = op_dest->val;    /* 提供补位比特的那一半 */
	DATA_TYPE out = op_src2->val;   /* 真正被移位、被写回的那一半 */

	uint8_t count = op_src->val;
	count &= 0x1f;
	while(count != 0) {
		out <<= 1;
		out |= MSB(in);
		in <<= 1;
		count --;
	}

	OPERAND_W(op_src2, out);

	print_asm("shld" str(SUFFIX) " %s,%s,%s", op_src->str, op_dest->str, op_src2->str);
}

make_helper(concat(shldi_, SUFFIX)) {
	int len = concat(decode_si_rm2r_, SUFFIX) (eip + 1);  /* 借用它读入 1 字节立即数 */
	op_dest->val = REG(op_dest->reg);
	do_execute();
	return len + 1;
}
#endif

#include "cpu/exec/template-end.h"
