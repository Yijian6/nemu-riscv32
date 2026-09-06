#ifndef __EFLAGS_H__
#define __EFLAGS_H__

#include "common.h"

void update_eflags_pf_zf_sf(uint32_t);

/* jcc/setcc 共用：cc 是 opcode 低4位选出的"条件码"(i386手册里的 tttn)，
 * 例如 0x4 = E/Z(相等), 0xc = L/NGE(有符号小于)。返回条件是否成立。 */
int check_cc(uint8_t cc);

#endif
