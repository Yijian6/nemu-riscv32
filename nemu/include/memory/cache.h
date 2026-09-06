#ifndef __CACHE_H__
#define __CACHE_H__

#include "common.h"

void init_cache(void);
uint32_t cache_read(hwaddr_t addr, size_t len);
void cache_write(hwaddr_t addr, size_t len, uint32_t data);

/* 模拟访存代价的统计量：命中一次记 2 个周期，缺失一次记 200 个周期。
 * 用来观察 cache 的效果(手册"观察 Cache 的作用"一节)，对功能本身没有影响。 */
extern uint64_t cache_cycle;
extern uint64_t cache_hit_cnt;
extern uint64_t cache_miss_cnt;

#endif
