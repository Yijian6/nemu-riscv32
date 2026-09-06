#ifndef __CALL_H__
#define __CALL_H__

make_helper(call_si_l);   /* CALL rel32  0xe8    ：相对调用 */
make_helper(call_rm_l);   /* CALL r/m32  0xff /2 ：间接(函数指针)调用 */

#endif
