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

	/* the first 4 bytes of an ELF file are 0x7f 'E' 'L' 'F' */
	const uint32_t elf_magic = 0x464c457f;
	uint32_t *p_magic = (void *)buf;
	nemu_assert(*p_magic == elf_magic);

	/* Load each program segment */
	int i;
	for(i = 0; i < elf->e_phnum; i ++) {
		ph = (void *)(buf + elf->e_phoff + i * elf->e_phentsize);

		/* Scan the program header table, load each segment into memory */
		if(ph->p_type == PT_LOAD) {

#ifdef IA32_PAGE
			/* The program will run on paging: p_vaddr is a virtual
			 * address outside the kernel's address space, so allocate
			 * physical memory for it and set up its page tables first.
			 * mm_malloc() returns the physical base, which is also a
			 * valid kernel virtual address through the identity
			 * mapping below 0xc0000000. */
			uint32_t pa = mm_malloc(ph->p_vaddr, ph->p_memsz);

			/* copy the segment content to [VirtAddr, VirtAddr + FileSiz) */
			ramdisk_read((uint8_t *)pa, ph->p_offset, ph->p_filesz);

			/* zero the memory region [VirtAddr + FileSiz, VirtAddr + MemSiz) */
			memset((uint8_t *)pa + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
#else
			/* copy the segment content to [VirtAddr, VirtAddr + FileSiz) */
			ramdisk_read((uint8_t *)ph->p_vaddr, ph->p_offset, ph->p_filesz);

			/* zero the memory region [VirtAddr + FileSiz, VirtAddr + MemSiz) */
			memset((uint8_t *)ph->p_vaddr + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
#endif

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
