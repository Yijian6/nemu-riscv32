#include "cpu/exec/helper.h"

/* 只需要 32 位寄存器压栈这一种形式 */
#define DATA_BYTE 4
#include "push-template.h"
#undef DATA_BYTE
