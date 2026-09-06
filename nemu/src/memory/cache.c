#include "common.h"
#include "memory/cache.h"
#include <stdlib.h>

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

/* ---- cache 的"构造"参数：改这三个宏就能得到不同规格的 cache ---- *
 *
 * 一级 cache 的规格（必做任务1）：
 *   块(block)大小 64B      -> BLOCK_WIDTH = 6
 *   总容量        64KB     -> SIZE_WIDTH  = 16
 *   8 路组相联             -> WAY_WIDTH   = 3
 * 于是组数 = 总容量 / 块大小 / 路数 = 64KB / 64B / 8 = 128 组，
 * 也就是 SET_WIDTH = 16 - 6 - 3 = 7 位。
 *
 * 一个物理地址就按这三段拆开来看：
 *   |<---- tag 19 位 ---->|<-- 组号 7 位 -->|<- 块内偏移 6 位 ->|
 * 块内偏移决定"在这一块的第几个字节"，组号决定"该去哪一组找"，
 * tag 用来确认"这一块装的到底是不是我要的那块内存"。 */
#define CACHE_BLOCK_WIDTH 6
#define CACHE_SIZE_WIDTH 10
#define CACHE_WAY_WIDTH 3

#define CACHE_SET_WIDTH (CACHE_SIZE_WIDTH - CACHE_BLOCK_WIDTH - CACHE_WAY_WIDTH)
#define CACHE_TAG_WIDTH (32 - CACHE_SET_WIDTH - CACHE_BLOCK_WIDTH)

#define NR_BLOCK_BYTE (1 << CACHE_BLOCK_WIDTH)
#define NR_WAY (1 << CACHE_WAY_WIDTH)
#define NR_SET (1 << CACHE_SET_WIDTH)

#define BLOCK_MASK (NR_BLOCK_BYTE - 1)

typedef struct {
	uint8_t buf[NR_BLOCK_BYTE];
	uint32_t tag;
	bool valid;			/* 本任务只要求 valid 位；写直达所以不需要 dirty 位 */
} CacheBlock;

static CacheBlock cache[NR_SET][NR_WAY];

/* 把一个物理地址拆成 tag / 组号 / 块内偏移三段，用位域一次性完成，
 * 和 dram.c 里把地址拆成 rank/bank/row/col 是同一个套路。 */
typedef union {
	struct {
		uint32_t offset	: CACHE_BLOCK_WIDTH;
		uint32_t set	: CACHE_SET_WIDTH;
		uint32_t tag	: CACHE_TAG_WIDTH;
	};
	uint32_t addr;
} cache_addr;

uint64_t cache_cycle = 0;
uint64_t cache_hit_cnt = 0;
uint64_t cache_miss_cnt = 0;

void init_cache() {
	int i, j;
	for(i = 0; i < NR_SET; i ++) {
		for(j = 0; j < NR_WAY; j ++) {
			cache[i][j].valid = false;
		}
	}
	cache_cycle = 0;
	cache_hit_cnt = 0;
	cache_miss_cnt = 0;
}

/* 在一组的 NR_WAY 路里找 tag 匹配且有效的那一路，找不到返回 -1 */
static int cache_find(cache_addr temp) {
	int i;
	for(i = 0; i < NR_WAY; i ++) {
		if(cache[temp.set][i].valid && cache[temp.set][i].tag == temp.tag) {
			return i;
		}
	}
	return -1;
}

/* 缺失时：挑一路装进来。先找空位，全满则随机挑一路覆盖（随机替换算法）。
 * 因为是写直达，被覆盖的块内容一定和内存一致，直接丢掉即可，不用写回。 */
static int cache_fill(cache_addr temp) {
	int i, way = -1;
	for(i = 0; i < NR_WAY; i ++) {
		if(!cache[temp.set][i].valid) { way = i; break; }
	}
	if(way == -1) { way = rand() % NR_WAY; }

	/* 把整块(64B)从 DRAM 搬进来。dram_read 一次最多给 4 字节，所以要读多次。 */
	hwaddr_t block_addr = temp.addr & ~BLOCK_MASK;
	for(i = 0; i < NR_BLOCK_BYTE; i += 4) {
		*(uint32_t *)(cache[temp.set][way].buf + i) = dram_read(block_addr + i, 4);
	}

	cache[temp.set][way].tag = temp.tag;
	cache[temp.set][way].valid = true;
	return way;
}

/* 定位 addr 所在的块：命中就直接返回，缺失就先装填再返回。同时记账。 */
static uint8_t *cache_locate(hwaddr_t addr) {
	cache_addr temp;
	temp.addr = addr;

	int way = cache_find(temp);
	if(way != -1) {
		cache_hit_cnt ++;
		cache_cycle += 2;
	}
	else {
		cache_miss_cnt ++;
		cache_cycle += 200;
		way = cache_fill(temp);
	}
	return cache[temp.set][way].buf;
}

uint32_t cache_read(hwaddr_t addr, size_t len) {
	uint32_t offset = addr & BLOCK_MASK;
	uint8_t *block = cache_locate(addr);

	if(offset + len <= NR_BLOCK_BYTE) {
		return unalign_rw(block + offset, 4);
	}

	/* 这次访问跨过了块的边界，要分两次读、再把两段拼起来。
	 * 注意必须先把第一块的数据拷出来再去定位第二块——第二次定位可能
	 * 发生替换，正好把第一块挤掉。 */
	/* 一次访存最多 4 字节，所以用框架自带的 unalign 联合体做暂存正好够用，
	 * 也避开了把 uint8_t 数组强转成别的类型时的 strict-aliasing 告警。 */
	unalign temp;
	temp._4 = 0;
	uint32_t first = NR_BLOCK_BYTE - offset;
	memcpy(&temp, block + offset, first);

	block = cache_locate(addr + first);
	memcpy((uint8_t *)&temp + first, block, len - first);

	return temp._4;
}

void cache_write(hwaddr_t addr, size_t len, uint32_t data) {
	cache_addr temp;
	temp.addr = addr;

	/* write through(写直达)：每次写都直接写到 DRAM，保证内存永远是最新的。
	 * not write allocate(非写分配)：写缺失时不把这块调进 cache，只写内存。
	 * 两者配合的好处是 cache 里的内容永远和内存一致，替换时无需写回。 */
	int way = cache_find(temp);
	if(way != -1) {
		cache_hit_cnt ++;
		cache_cycle += 2;

		uint32_t offset = addr & BLOCK_MASK;
		if(offset + len <= NR_BLOCK_BYTE) {
			memcpy(cache[temp.set][way].buf + offset, &data, len);
		}
		else {
			/* 跨块：本块只放得下前一段，剩下那段属于下一块，
			 * 下一块在 cache 里就更新，不在就算了(非写分配)。 */
			uint32_t first = NR_BLOCK_BYTE - offset;
			memcpy(cache[temp.set][way].buf + offset, &data, first);

			cache_addr next;
			next.addr = addr + first;
			int next_way = cache_find(next);
			if(next_way != -1) {
				memcpy(cache[next.set][next_way].buf, (uint8_t *)&data + first, len - first);
			}
		}
	}
	else {
		cache_miss_cnt ++;
		cache_cycle += 200;
	}

	dram_write(addr, len, data);
}
