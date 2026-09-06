#ifndef __RET_H__
#define __RET_H__

make_helper(ret);     /* RET       0xc2 之外的无操作数形式 0xc3 */
make_helper(ret_i);   /* RET imm16 0xc2：返回并弹掉 imm16 字节的实参 */

#endif
