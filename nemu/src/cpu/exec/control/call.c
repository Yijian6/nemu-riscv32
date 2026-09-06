#include "cpu/exec/helper.h"

/* x86 的 CALL rel32 只有 32 位偏移量这一种相对调用形式（没有 rel8/rel16 的近调用），
 * 所以只实例化一次，DATA_BYTE = 4。 */
#define DATA_BYTE 4
#include "call-template.h"
#undef DATA_BYTE
