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
#include <x86/x86-linux.h>
#include "kexec.h"
#include "kexec-x86.h"

int bzImage_probe(FILE *file)
{
	struct x86_linux_header header;
	if (fseek(file, 0, SEEK_SET) < 0) {
		fprintf(stderr, "seek error: %s\n",
			strerror(errno));
		return -1;
	}
	if (fread(&header, sizeof(header), 1, file)  != 1) {
		fprintf(stderr, "read error: %s\n",
			strerror(errno));
		return -1;
	}
	if (memcmp(header.header_magic, "HdrS", 4) != 0) {
		fprintf(stderr, "Not a bzImage\n");
		return -1;
	}
	if (header.boot_sector_magic != 0xAA55) {
		/* No x86 boot sector present */
		return 0;
	}
	if (header.protocol_version < 0x0200) {
		/* Must be at least protocol version 2.00 */
		return 0;
	}
	if ((header.loadflags & 1) == 0) {
		/* Not a bzImage */
		return 0;
	}
	/* I've got a bzImage */
	return 1;
}


void bzImage_usage(void)
{
	printf(	"-d, --debug               Enable debugging to help spot a failure.\n"
		"    --real-mode           Use the kernels real mode entry point.\n"
		"    --command-line=STRING Set the kernel command line to STRING.\n"
		"    --append=STRING       Set the kernel command line to STRING.\n"
		"    --initrd=FILE         Use FILE as the kernel's initial ramdisk.\n"
		"    --ramdisk=FILE        Use FILE as the kernel's initial ramdisk.\n"
		);
       
}

int setup_linux_parameters(struct x86_linux_param_header *real_mode)
{
	struct memory_range *range;
	int i, ranges;
	
	/* Default screen size */
	real_mode->orig_x = 0;
	real_mode->orig_y = 0;
	real_mode->orig_video_page = 0;
	real_mode->orig_video_mode = 0;
	real_mode->orig_video_cols = 80;
	real_mode->orig_video_lines = 25;
	real_mode->orig_video_ega_bx = 0;
	real_mode->orig_video_isVGA = 1;
	real_mode->orig_video_points = 16;

	/* Fill in the memsize later */
	real_mode->ext_mem_k = 0;
	real_mode->alt_mem_k = 0;
	real_mode->e820_map_nr = 0;

	/* Default APM info */
	memset(&real_mode->apm_bios_info, 0, sizeof(real_mode->apm_bios_info));
	/* Default drive info */
	memset(&real_mode->drive_info, 0, sizeof(real_mode->drive_info));
	/* Default sysdesc table */
	real_mode->sys_desc_table.length = 0;

	/* default yes: this can be overridden on the command line */
	real_mode->mount_root_rdonly = 0xFFFF;

	/* default /dev/hda
	 * this can be overrident on the command line if necessary.
	 */
	real_mode->root_dev = (0x3 <<8)| 0;

	/* another safe default */
	real_mode->aux_device_info = 0;

	/* Fill in the memory info */
	if ((get_memory_ranges(&range, &ranges) < 0) || ranges == 0) {
		fprintf(stderr, "Cannot get memory information\n");
		return -1;
	}
	if (ranges > E820MAX) {
		fprintf(stderr, "Too many memory ranges, truncating...\n");
		ranges = E820MAX;
	}
	real_mode->e820_map_nr = ranges;
	for(i = 0; i < ranges; i++) {
		real_mode->e820_map[i].addr = range[i].start;
		real_mode->e820_map[i].size = range[i].end - range[i].start;
		switch (range[i].type) {
		case RANGE_RAM:
			real_mode->e820_map[i].type = E820_RAM; 
			break;
		case RANGE_ACPI:
			real_mode->e820_map[i].type = E820_ACPI; 
			break;
		case RANGE_ACPI_NVS:
			real_mode->e820_map[i].type = E820_NVS;
			break;
		default:
		case RANGE_RESERVED:
			real_mode->e820_map[i].type = E820_RESERVED; 
			break;
		}
		if (range[i].type != RANGE_RAM)
			continue;
		if ((range[i].start <= 0x100000) && range[i].end > 0x100000) {
			unsigned long long mem_k = (range[i].end >> 10) - 0x100000;
			real_mode->ext_mem_k = mem_k;
			real_mode->alt_mem_k = mem_k;
			if (mem_k > 0xfc00) {
				real_mode->ext_mem_k = 0xfc00; /* 64M */
			}
			if (mem_k > 0xffffffff) {
				real_mode->alt_mem_k = 0xffffffff;
			}
		}
	}
	return 0;
}

int bzImage_load(FILE *file, int argc, char **argv,
	void **ret_entry, struct kexec_segment **ret_segments, int *ret_nr_segments)
{
	struct x86_linux_header header;
	struct x86_linux_param_header *real_mode;
	int setup_sects;
	char *kernel_version;
	struct kexec_segment *segment;
	int nr_segments;
	long length;
	size_t size;
	const char *command_line;
	const char *ramdisk;
	FILE *fp_ramdisk;
	unsigned long ramdisk_length;
	int command_line_len;
	int command_line_off;
	int kern16_size;
	int setup16_off, setup32_off;
	char *cmdline;
	char *start16, *start32;
	unsigned long entry;
	int debug, real_mode_entry;
	int opt;
#define OPT_APPEND	OPT_MAX+0
#define OPT_INITRD	OPT_MAX+1
#define OPT_RAMDISK	OPT_MAX+2
#define OPT_REAL_MODE	OPT_MAX+3
	static const struct option options[] = {
		KEXEC_OPTIONS
		{ "debug",		0, 0, OPT_DEBUG },
		{ "command-line",	1, 0, OPT_APPEND },
		{ "append",		1, 0, OPT_APPEND },
		{ "initrd",		1, 0, OPT_RAMDISK },
		{ "ramdisk",		1, 0, OPT_RAMDISK },
		{ "real-mode",		0, 0, OPT_REAL_MODE },
		{ 0, 			0, 0, 0 },
	};
	static const char short_options[] = KEXEC_OPT_STR "d";

	/*
	 * Parse the command line arguments
	 */
	debug = 0;
	real_mode_entry = 0;
	command_line = 0;
	ramdisk = 0;
	ramdisk_length = 0;
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
		case OPT_DEBUG:
			debug = 1;
			break;
		case OPT_APPEND:
			command_line = optarg;
			break;
		case OPT_RAMDISK:
			ramdisk = optarg;
			break;
		case OPT_REAL_MODE:
			real_mode_entry = 1;
			break;
		}
	}
	command_line_len = 0;
	if (command_line) {
		command_line_len = strlen(command_line) +1;
	}
	if (ramdisk) {
		fp_ramdisk = fopen(ramdisk, "rb");
		if (!fp_ramdisk) {
			fprintf(stderr, "Cannot open %s:%s\n",
				ramdisk, strerror(errno));
			return -1;
		}
		if (fseek(fp_ramdisk, 0, SEEK_END) < 0) {
			fprintf(stderr, "seek error on %s: %s\n",
				ramdisk, strerror(errno));
			return -1;
		}
		ramdisk_length = ftell(fp_ramdisk);
	}
	/*
	 * Find out about the file I am about to load.
	 */
	if (fseek(file, 0, SEEK_SET) < 0) {
		fprintf(stderr, "seek error: %s\n",
			strerror(errno));
		return -1;
	}
	if (fread(&header, sizeof(header), 1, file)  != 1) {
		fprintf(stderr, "read error: %s\n",
			strerror(errno));
		return -1;
	}
	setup_sects = header.setup_sects;
	if (setup_sects == 0) {
		setup_sects = 4;
	}
	kern16_size = (setup_sects +1) *512;
	kernel_version = ((unsigned char *)&header) + 512 + header.kver_addr;
	nr_segments = ramdisk?3:2;
	segment = malloc(nr_segments *sizeof(*segment));
	if (segment == 0) {
		fprintf(stderr, "malloc failed: %s\n",
			strerror(errno));
		return -1;
	}
	/* The x86 code segment */
	command_line_off = kern16_size;
	if (!debug) {
		setup16_off = (command_line_off + command_line_len + setup16_align -1) & 
			~(setup16_align -1);
		size = setup16_off + setup16_size;
	} else {
		setup16_off = (command_line_off + command_line_len + setup16_debug_align -1) & 
			~(setup16_debug_align -1);
		size = setup16_off + setup16_debug_size;
	}
	/* The 32bit entry point */
	setup32_off = (size + 3) & ~3; /* 4 byte align */
	size = setup32_off + setup32_size;
	
	/* Remember where the segments will live */
	segment[0].bufsz = size;
	segment[0].memsz = size;
	segment[0].mem = (void *)0x90000;
	segment[0].buf = malloc(size);
	if (segment[0].buf == 0) {
		fprintf(stderr, "malloc failed: %s\n",
			strerror(errno));
		return -1;
	}
	if (fseek(file, 0, SEEK_END) < 0) {
		fprintf(stderr, "seek error: %s\n",
			strerror(errno));
		return -1;
	}
	length = ftell(file);
	size = length - kern16_size;
	segment[1].bufsz = size;
	segment[1].memsz = size;
	segment[1].mem = (void *)0x100000; /* 1MB */
	segment[1].buf = malloc(size);
	if (segment[1].buf == 0) {
		fprintf(stderr, "malloc failed: %s\n",
			strerror(errno));
		return -1;
	}
	segment[2].bufsz = ramdisk_length;
	segment[2].memsz = ramdisk_length;
	segment[2].mem = (void *)0x800000; /* 8MB */
	segment[2].buf = malloc(segment[2].bufsz);
	if (segment[2].buf == 0) {
		fprintf(stderr, "malloc failed: %s\n",
			strerror(errno));
		return -1;
	}
	real_mode = segment[0].buf;
	cmdline = (char *)segment[0].buf + command_line_off;
	start16 =(char *)segment[0].buf + setup16_off;
	start32 =(char *)segment[0].buf + setup32_off;
	if (real_mode_entry) {
		printf("Starting the kernel in real mode\n");
		entry = (unsigned long)segment[0].mem + setup16_off;
	} else {
		entry = (unsigned long)segment[0].mem + setup32_off;
	}
	if (debug) {
		printf("setup16_end: %08x\n", 0x90000 + setup16_off + setup16_debug_size);
	}
	if (fseek(file, 0, SEEK_SET) < 0) {
		fprintf(stderr, "seek error: %s\n",
			strerror(errno));
		return -1;
	}
	if (fread(segment[0].buf, kern16_size, 1, file)  != 1) {
		fprintf(stderr, "read error: %s\n",
			strerror(errno));
		return -1;
	}
	/* 
	 * Initialize the param_header with bootloader information.
	 */
	/* The location of the command line */
	real_mode->cl_magic = CL_MAGIC_VALUE;
	real_mode->cl_offset = kern16_size;
	if (header.protocol_version >= 0x0202) {
		real_mode->cmd_line_ptr = 0x90000 + command_line_off;
	}
	/* The loader type */
	real_mode->loader_type = LOADER_TYPE_UNKNOWN;
	/* The ramdisk */
	real_mode->initrd_start = 0;
	real_mode->initrd_size = 0;
	if (ramdisk) {
		real_mode->initrd_start = (unsigned long)segment[2].mem;
		real_mode->initrd_size = segment[2].memsz;
	}
	/* debug hooks */
	if (debug) {
		/* Set the pre protected mode switch routine */
		real_mode->rmode_switch_ip = setup16_off + 
			(setup16_debug_kernel_pre_protected - setup16_debug_start);
		real_mode->rmode_switch_cs = 0x9000;

		/* Set up the post protected mode switch routine */
		setup16_debug_old_code32 = real_mode->kernel_start;
		real_mode->kernel_start = 0x90000 + setup16_off + 
			(setup16_debug_first_code32 - setup16_debug_start);
	}
	/*
	 * Initialize the 16bit start information.
	 */
	if (!debug) {
		memset(&setup16_regs, 0, sizeof(setup16_regs));
		setup16_regs.cs = 0x9020;
		setup16_regs.ip = 0;
		setup16_regs.ss = 0x8000;
		setup16_regs.esp = 0xFFFC;
	} else {
		memset(&setup16_debug_regs, 0, sizeof(setup16_debug_regs));
		setup16_debug_regs.cs = 0x9020;
		setup16_debug_regs.ip = 0;
		setup16_debug_regs.ss = 0x8000;
		setup16_debug_regs.esp = 0xFFFC;
	}
	/*
	 * Initialize the 32bit start information.
	 */
	setup32_regs.eax = 0; /* unused */
	setup32_regs.ebx = 0; /* 0 == boot not AP processor start */
	setup32_regs.ecx = 0; /* unused */
	setup32_regs.edx = 0; /* unused */
	setup32_regs.esi = (unsigned long)segment[0].mem; /* kernel parameters */
	setup32_regs.edi = 0; /* unused */
	setup32_regs.esp = (unsigned long)segment[0].mem; /* stack, unused */
	setup32_regs.ebp = 0; /* unused */
	setup32_regs.eip = (unsigned long)segment[1].mem; /* kernel entry point */

	/*
	 * Copy it all into the startup vector
	 */
	memcpy(cmdline, command_line, command_line_len);
	if (!debug) {
		memcpy(start16, setup16_start, setup16_size);
	} else {
		memcpy(start16, setup16_debug_start, setup16_debug_size);
	}
	memcpy(start32, setup32_start, setup32_size);

	/* Fill in the information BIOS calls would normally provide. */
	if (!real_mode_entry) {
		if (setup_linux_parameters(real_mode) < 0) {
			return -1;
		}
	}

	/* The 32bit data */
	if (fseek(file, kern16_size, SEEK_SET) < 0) {
		fprintf(stderr, "seek error: %s\n",
			strerror(errno));
		return -1;
	}
	if (fread(segment[1].buf, segment[1].bufsz, 1, file)  != 1) {
		fprintf(stderr, "read error: %s\n",
			strerror(errno));
		return -1;
	}
	/* The ramdisk */
	if (ramdisk) {
		if (fseek(fp_ramdisk, 0, SEEK_SET) < 0) {
			fprintf(stderr, "seek error: %s\n",
				strerror(errno));
			return -1;
		}
		if (fread(segment[2].buf, ramdisk_length, 1, fp_ramdisk) != 1) {
			fprintf(stderr, "read error: %s\n",
				strerror(errno));
			return -1;
		}
	}
	*ret_entry = (void *)entry;
	*ret_nr_segments = nr_segments;
	*ret_segments = segment;
	return 0;
}
