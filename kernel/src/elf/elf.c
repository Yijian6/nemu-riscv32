#include "common.h"
#include "memory.h"
#include <string.h>
#include <elf.h>

#define ELF_OFFSET_IN_DISK 0

#ifdef HAS_DEVICE
void ide_read(uint8_t *, uint32_t, uint32_t);
#else
void ramdisk_read(uint8_t *, uint32_t, uint32_t);
#endif

#define STACK_SIZE (1 << 20)

void create_video_mapping();
uint32_t get_ucr3();

uint32_t loader() {
	Elf32_Ehdr *elf;
	Elf32_Phdr *ph = NULL;

	uint8_t buf[4096];

#ifdef HAS_DEVICE
	ide_read(buf, ELF_OFFSET_IN_DISK, 4096);
#else
	ramdisk_read(buf, ELF_OFFSET_IN_DISK, 4096);
#endif

	elf = (void*)buf;

	/* ELF 文件开头的 4 个"魔数"字节是 0x7f 'E' 'L' 'F'，即 7f 45 4c 46。
	 * x86 是小端序，把这 4 个字节当成一个 32 位整数读出来时字节序要反过来，
	 * 所以比较的常量是 0x464c457f。（这也回答了思考题5：操作系统就是靠
	 * 文件开头这几个字节来判断"格式错误"的。） */
	const uint32_t elf_magic = 0x464c457f;
	uint32_t *p_magic = (void *)buf;
	nemu_assert(*p_magic == elf_magic);

	/* Load each program segment */

	/* program header table(程序头表)描述了"面向执行"的视角：每一个表项
	 * 描述一个 segment(段)。表从文件偏移 e_phoff 处开始，共 e_phnum 项，
	 * 每项 e_phentsize 字节。 */
	int i;
	for(i = 0; i < elf->e_phnum; i ++) {
		ph = (Elf32_Phdr *)(buf + elf->e_phoff + i * elf->e_phentsize);

		/* Scan the program header table, load each segment into memory */
		if(ph->p_type == PT_LOAD) {

			/* 把 segment 的内容从 ELF 文件(在 ramdisk 里)搬到内存
			 * [VirtAddr, VirtAddr + FileSiz)。现在还没有虚拟内存，
			 * VirtAddr 直接当物理地址用。 */
			ramdisk_read((uint8_t *)ph->p_vaddr, ph->p_offset, ph->p_filesz);

			/* 把 [VirtAddr + FileSiz, VirtAddr + MemSiz) 清零。
			 * 这段差额就是思考题6 的答案：像未初始化全局变量(.bss)这类数据，
			 * 内容全是 0，没必要在文件里存一大片 0 白白占空间，只要记下
			 * "在内存里要占 MemSiz 这么大"，加载时补零即可。所以 FileSiz
			 * 永远不会大于 MemSiz。 */
			memset((void *)(ph->p_vaddr + ph->p_filesz), 0,
					ph->p_memsz - ph->p_filesz);

#ifdef IA32_PAGE
			/* Record the program break for future use. */
			extern uint32_t cur_brk, max_brk;
			uint32_t new_brk = ph->p_vaddr + ph->p_memsz - 1;
			if(cur_brk < new_brk) { max_brk = cur_brk = new_brk; }
#endif
		}
	}

	volatile uint32_t entry = elf->e_entry;

#ifdef IA32_PAGE
	mm_malloc(KOFFSET - STACK_SIZE, STACK_SIZE);

#ifdef HAS_DEVICE
	create_video_mapping();
#endif

	write_cr3(get_ucr3());
#endif

	return entry;
}
