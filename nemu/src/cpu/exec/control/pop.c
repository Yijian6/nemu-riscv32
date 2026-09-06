#include "cpu/exec/helper.h"

/* 只需要 32 位寄存器弹栈这一种形式 */
#define DATA_BYTE 4
#include "pop-template.h"
#undef DATA_BYTE
