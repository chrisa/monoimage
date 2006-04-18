#ifndef KEXEC_X86_H
#define KEXEC_X86_H

extern unsigned char compat_x86_64[];
extern uint32_t compat_x86_64_size, compat_x86_64_entry32;

struct entry32_regs {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t esp;
	uint32_t ebp;
	uint32_t eip;
};

struct entry16_regs {
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
	uint16_t pad;
};

int multiboot_x86_probe(const char *buf, off_t len);
int multiboot_x86_load(int argc, char **argv, const char *buf, off_t len,
	struct kexec_info *info);
void multiboot_x86_usage(void);

int elf_x86_probe(const char *buf, off_t len);
int elf_x86_load(int argc, char **argv, const char *buf, off_t len,
	struct kexec_info *info);
void elf_x86_usage(void);

int bzImage_probe(const char *buf, off_t len);
int bzImage_load(int argc, char **argv, const char *buf, off_t len, 
	struct kexec_info *info);
void bzImage_usage(void);

int beoboot_probe(const char *buf, off_t len);
int beoboot_load(int argc, char **argv, const char *buf, off_t len,
	struct kexec_info *info);
void beoboot_usage(void);

int nbi_probe(const char *buf, off_t len);
int nbi_load(int argc, char **argv, const char *buf, off_t len,
	struct kexec_info *info);
void nbi_usage(void);

int monoimage_probe(const char *buf, off_t len);
int monoimage_load(int argc, char **argv, const char *buf, off_t len,
	struct kexec_info *info);
void monoimage_usage(void);
#endif /* KEXEC_X86_H */
