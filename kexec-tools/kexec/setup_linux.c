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
