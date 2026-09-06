#ifndef __JCC_H__
#define __JCC_H__

/* JE 之外，条件跳转的完整 16 种(je/jne/jl/jle/jg/jge/jb/jbe/ja/jae/...)：
 * 短跳(rel8, opcode 0x70-0x7f)和近跳(rel32, opcode 0x0f 0x80-0x8f)各是
 * "同一个函数"，靠 opcode 低4位在运行时选条件，见 jcc-template.h。 */
make_helper(jcc_si_b);
make_helper(jcc_si_l);

#endif
