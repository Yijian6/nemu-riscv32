#include "cpu/reg.h"
#include "debug.h"

static const int parity_table [] = {
	0, 1, 0, 1,
	1, 0, 1, 0,
	0, 1, 0, 1,
	1, 0, 1, 0
};

void update_eflags_pf_zf_sf(uint32_t result) {
	uint8_t temp = result & 0xff;
	cpu.eflags.PF = parity_table[temp & 0xf] ^ parity_table[temp >> 4];
	cpu.eflags.ZF = (result == 0);
	cpu.eflags.SF = result >> 31;
}

const char *cc_name[16] = {
	"o", "no", "b", "ae", "e", "ne", "be", "a",
	"s", "ns", "p", "np", "l", "ge", "le", "g"
};

/* i386 手册第20章 jcc 表：16 种条件码(tttn)，jcc/setcc 共用同一套判断，
 * 只是"跳不跳"和"写不写1个字节"的区别。 */
int check_cc(uint8_t cc) {
	switch (cc & 0xf) {
		case 0x0: return cpu.eflags.OF;                                        /* O */
		case 0x1: return !cpu.eflags.OF;                                       /* NO */
		case 0x2: return cpu.eflags.CF;                                        /* B/C/NAE */
		case 0x3: return !cpu.eflags.CF;                                       /* NB/NC/AE */
		case 0x4: return cpu.eflags.ZF;                                        /* E/Z */
		case 0x5: return !cpu.eflags.ZF;                                       /* NE/NZ */
		case 0x6: return cpu.eflags.CF || cpu.eflags.ZF;                       /* BE/NA */
		case 0x7: return !cpu.eflags.CF && !cpu.eflags.ZF;                     /* NBE/A */
		case 0x8: return cpu.eflags.SF;                                        /* S */
		case 0x9: return !cpu.eflags.SF;                                       /* NS */
		case 0xa: return cpu.eflags.PF;                                        /* P/PE */
		case 0xb: return !cpu.eflags.PF;                                       /* NP/PO */
		case 0xc: return cpu.eflags.SF != cpu.eflags.OF;                       /* L/NGE */
		case 0xd: return cpu.eflags.SF == cpu.eflags.OF;                       /* NL/GE */
		case 0xe: return cpu.eflags.ZF || (cpu.eflags.SF != cpu.eflags.OF);    /* LE/NG */
		case 0xf: return !cpu.eflags.ZF && (cpu.eflags.SF == cpu.eflags.OF);   /* NLE/G */
		default: assert(0);
	}
}
