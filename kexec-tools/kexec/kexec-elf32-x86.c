#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <elf.h>
#include <boot/elf_boot.h>
#include <ip_checksum.h>
#include "kexec.h"
#include "kexec-x86.h"


#define BOOTLOADER "kexec"
#define BOOTLOADER_VERSION VERSION
#define MAX_COMMAND_LINE 256

#define UPSZ(X) ((sizeof(X) + 3) &~3)
static struct boot_notes {
	Elf_Bhdr hdr;
	Elf_Nhdr bl_hdr;
	unsigned char bl_desc[UPSZ(BOOTLOADER)];
	Elf_Nhdr blv_hdr;
	unsigned char blv_desc[UPSZ(BOOTLOADER_VERSION)];
	Elf_Nhdr cmd_hdr;
	unsigned char command_line[0];
} 
elf_boot_notes = {
	.hdr = {
		.b_signature = 0x0E1FB007,
		.b_size = sizeof(elf_boot_notes),
		.b_checksum = 0,
		.b_records = 3,
	},
	.bl_hdr = {
		.n_namesz = 0,
		.n_descsz = sizeof(BOOTLOADER),
		.n_type = EBN_BOOTLOADER_NAME,
	},
	.bl_desc = BOOTLOADER,
	.blv_hdr = {
		.n_namesz = 0,
		.n_descsz = sizeof(BOOTLOADER_VERSION),
		.n_type = EBN_BOOTLOADER_VERSION,
	},
	.blv_desc = BOOTLOADER_VERSION,
	.cmd_hdr = {
		.n_namesz = 0,
		.n_descsz = 0,
		.n_type = EBN_COMMAND_LINE,
	},
};


int elf32_x86_probe(FILE *file)
{
	Elf32_Ehdr ehdr;
	if (fseek(file, 0, SEEK_SET) < 0) {
		fprintf(stderr, "seek error: %s\n",
			strerror(errno));
		return -1;
	}
	if (fread(&ehdr, sizeof(ehdr), 1, file)  != 1) {
		fprintf(stderr, "read error: %s\n",
			strerror(errno));
		return -1;
	}
	if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
		/* No ELF header */
		return 0;
	}
	if (ehdr.e_ident[EI_CLASS] != ELFCLASS32) {
		/* Not a 32bit ELF file */
		return 0;
	}
	if (ehdr.e_ident[EI_DATA] != ELFDATA2LSB) {
		/* not a little endian ELF file */
		return 0;
	}
	if ((ehdr.e_ident[EI_VERSION] != EV_CURRENT) ||
		(ehdr.e_version != EV_CURRENT)) {
		/* unkown elf version */
		return 0;
	}
	if (ehdr.e_type != ET_EXEC) {
		/* not an ELF executable */
		return 0;
	}

	if (ehdr.e_ehsize != sizeof(Elf32_Ehdr)) {
		/* invalid ELF header size */
		return 0;
	}
	if (ehdr.e_phentsize != sizeof(Elf32_Phdr)) {
		/* invalid program header size */
		return 0;
	}
	if ((ehdr.e_phoff == 0) || (ehdr.e_phnum == 0)) {
		/* no program header */
		return 0;
	}
	/* Verify the architecuture specific bits */
	if ((ehdr.e_machine != EM_386) && (ehdr.e_machine != EM_486)) {
		/* for a different architecture */
		return 0;
	}
	return 1;
}

void elf32_x86_usage(void)
{
	printf(	"    --command-line=STRING\n");
}

int elf32_x86_load(FILE *file, int argc, char **argv,
	void **ret_entry, struct kexec_segment **ret_segments, int *ret_nr_segments)
{
	struct kexec_segment *segment;
	int nr_segments;
	Elf32_Ehdr ehdr;
	Elf32_Phdr *phdr;
	size_t phdr_bytes;
	size_t arg_bytes;
	struct boot_notes *notes;
	size_t note_bytes;
	const char *command_line;
	int command_line_len;
	int i;
	int opt;
#define OPT_APPEND	OPT_MAX+0
#define OPT_INITRD	OPT_MAX+1
#define OPT_RAMDISK	OPT_MAX+2
	static const struct option options[] = {
		KEXEC_OPTIONS
		{ "command-line",	1, 0, OPT_APPEND },
		{ "append",		1, 0, OPT_APPEND },
		{ 0, 			0, 0, 0 },
	};

	static const char short_options[] = KEXEC_OPT_STR "";

	command_line = 0;
	while((opt = getopt_long(argc, argv, short_options, options, 0)) != -1) {
		switch(opt) {
		default:
			/* Ignore core options */
			if (opt < OPT_MAX) {
				break;
			}
		case '?':
			usage();
			return -1;
		case OPT_APPEND:
			command_line = optarg;
			break;
		}
	}
	command_line_len = 0;
	if (command_line) {
		command_line_len = strlen(command_line) +1;
	}

	/* Read in the Elf header */
	if (fseek(file, 0, SEEK_SET) != 0) {
		fprintf(stderr, "seek error: %s\n",
			strerror(errno));
		return -1;
	}
	if (fread(&ehdr, sizeof(ehdr), 1, file) != 1) {
		fprintf(stderr, "read error: %s\n",
			strerror(errno));
		return -1;
	}
	/* Read in the program header */
	phdr_bytes = sizeof(*phdr) *ehdr.e_phnum;
	phdr = malloc(phdr_bytes);
	if (phdr == 0) {
		fprintf(stderr, "malloc failed: %s\n",
			strerror(errno));
		return -1;
	}
	if (fseek(file, ehdr.e_phoff, SEEK_SET) != 0) {
		fprintf(stderr, "seek error: %s\n",
			strerror(errno));
		return -1;
	}
	if (fread(phdr, phdr_bytes, 1, file) != 1) {
		fprintf(stderr, "read error: %s\n",
			strerror(errno));
		return -1;
	}

	/* Setup the segments */
	segment = malloc(sizeof(*segment) * (ehdr.e_phnum +1));
	if (segment == 0) {
		fprintf(stderr, "malloc failed: %s\n",
			strerror(errno));
		return -1;
	}

	/* Skip the argument segment */
	nr_segments = 0;
	segment[nr_segments].buf = 0;
	segment[nr_segments].bufsz = 0;
	segment[nr_segments].mem = 0;
	segment[nr_segments].memsz = 0;
	nr_segments++;

	/* Now all the rest of the segments */
	for(i = 0; i < ehdr.e_phnum; i++) {
		char *buf;
		size_t size;
		if (phdr[i].p_type != PT_LOAD) {
			continue;
		}
		size = phdr[i].p_filesz;
		if (size > phdr[i].p_memsz) {
			size = phdr[i].p_memsz;
		}
		buf = malloc(size);
		if (buf == 0) {
			fprintf(stderr, "malloc failed: %s\n",
				strerror(errno));
			return -1;
		}
		segment[nr_segments].buf = buf;
		segment[nr_segments].bufsz = size;
		segment[nr_segments].mem = (void *)phdr[i].p_paddr;
		segment[nr_segments].memsz = phdr[i].p_memsz;
		if (valid_memory_range(segment + nr_segments) < 0) {
			fprintf(stderr, "Invalid memory segment %p - %p\n",
				segment[nr_segments].mem,
				((char*)segment[nr_segments].mem) + segment[nr_segments].memsz);
			return -1;
		}
		nr_segments++;
		if (size == 0) {
			/* Don't do file I/O if there is nothing in the file */
			continue;
		}
		if (fseek(file, phdr[i].p_offset, SEEK_SET) != 0) {
			fprintf(stderr, "seek failed: %s\n",
				strerror(errno));
			return -1;
		}
		if (fread(buf, size, 1, file) != 1) {
			fprintf(stderr, "Read failed: %s\n",
				strerror(errno));
			return -1;
		}
	}
	/* Generate and setup the argument segment */
	if (sort_segments(segment, nr_segments) < 0) {
		return -1;
	}
	note_bytes = sizeof(elf_boot_notes) + ((command_line_len + 3) & ~3);
	arg_bytes = note_bytes + ((setup32_size + 3) & ~3);
	segment[0].bufsz = arg_bytes;
	segment[0].memsz = arg_bytes;
	if (locate_arguments(segment, nr_segments, 4, 0xFFFFFFFFUL) < 0) {
		return -1;
	}
	segment[0].buf = malloc(arg_bytes);
	if (segment[0].buf == 0) {
		fprintf(stderr, "malloc failed: %s\n",
			strerror(errno));
		return -1;
	}
	notes = (struct boot_notes *)(((char *)segment[0].buf) + ((setup32_size + 3) & ~3));
	setup32_regs.eax = 0x0E1FB007;
	setup32_regs.ebx = ((unsigned long)segment[0].mem) + ((setup32_size + 3) & ~3);
	setup32_regs.eip = ehdr.e_entry;
	memcpy(segment[0].buf, setup32_start, setup32_size);
	memcpy(notes, &elf_boot_notes, sizeof(elf_boot_notes));
	memcpy(notes->command_line, command_line, command_line_len);
	notes->hdr.b_size = note_bytes;
	notes->cmd_hdr.n_descsz = command_line_len;
	notes->hdr.b_checksum = compute_ip_checksum(notes, note_bytes);

	*ret_entry = segment[0].mem;
	*ret_nr_segments = nr_segments;
	*ret_segments = segment;
	return 0;
}
