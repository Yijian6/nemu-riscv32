#include "cpu/exec/template-start.h"

#define instr lods

/* LODS：把 [esi] 处的数据装进 AL/AX/EAX，然后 esi 前进(DF=0)或后退(DF=1)
 * 一个数据宽度。和已有的 stos(存)/scas(比)/movs(搬)是同一族，只是方向不同：
 * 这条是"取"。strlen/strcmp 这类字符串函数编译后常见。 */
make_helper(concat(lods_, SUFFIX)) {
	REG(R_EAX) = MEM_R(cpu.esi);
	cpu.esi += (cpu.eflags.DF ? -DATA_BYTE : DATA_BYTE);

	print_asm("lods" str(SUFFIX) " %%ds:(%%esi),%%%s", REG_NAME(R_EAX));
	return 1;
}

#include "cpu/exec/template-end.h"
