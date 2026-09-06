#ifndef __CMP_H__
#define __CMP_H__

/* CMP r/m32, imm8 (sign-extended)：mov-c 唯一用到的形式，按 KISS 原则只实现这一种 */
make_helper(cmp_si2rm_w);
make_helper(cmp_si2rm_l);

make_helper(cmp_si2rm_v);

#endif
