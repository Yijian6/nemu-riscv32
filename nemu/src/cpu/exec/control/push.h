#ifndef __PUSH_H__
#define __PUSH_H__

make_helper(push_r_l);    /* PUSH r32   0x50-0x57 */
make_helper(push_rm_l);   /* PUSH r/m32 0xff /6   */
make_helper(push_i_l);    /* PUSH imm32 0x68      */
make_helper(push_si_b);   /* PUSH imm8  0x6a      */

#endif
