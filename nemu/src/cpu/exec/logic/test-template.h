#include "cpu/exec/template-start.h"

#define instr test

static void do_execute () {
	DATA_TYPE result = op_dest->val & op_src->val;

	update_eflags_pf_zf_sf((DATA_TYPE_S)result);
	cpu.eflags.CF = cpu.eflags.OF = 0;

	print_asm_template2();
}

make_instr_helper(i2a)     /* TEST AL/eAX, imm : 0xa8 / 0xa9 */
make_instr_helper(i2rm)    /* TEST r/m, imm    : 0xf6 /0 / 0xf7 /0 */
make_instr_helper(r2rm)    /* TEST r/m, r      : 0x84 / 0x85 */

#include "cpu/exec/template-end.h"
