#ifndef KEXEC_X86_H
#define KEXEC_X86_H

extern unsigned char setup32_start[], setup16_start[];
extern uint32_t setup32_size, setup16_size, setup16_align;

extern unsigned char setup16_debug_start[];
extern unsigned char setup16_debug_kernel_pre_protected[];
extern unsigned char setup16_debug_first_code32[];
extern uint32_t setup16_debug_size, setup16_debug_align, setup16_debug_old_code32;

extern struct {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t esp;
	uint32_t ebp;
	uint32_t eip;
} setup32_regs;

extern struct {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t esp;
	uint32_t ebp;
	uint16_t ds;
	uint16_t es;
	uint16_t ss;
	uint16_t fs;
	uint16_t gs;
	uint16_t ip;
	uint16_t cs;
} setup16_regs, setup16_debug_regs;

int elf32_x86_probe(FILE *file);
int elf32_x86_load(FILE *file, int argc, char **argv,
	void **ret_entry, struct kexec_segment **ret_segments, int *ret_nr_segments);
void elf32_x86_usage(void);

int bzImage_probe(FILE *file);
int bzImage_load(FILE *file, int argc, char **argv,
	void **ret_entry, struct kexec_segment **ret_segments, int *ret_nr_segments);
void bzImage_usage(void);

#endif /* KEXEC_X86_H */
