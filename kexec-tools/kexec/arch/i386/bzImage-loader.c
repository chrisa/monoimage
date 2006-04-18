/*
 * kexec: Linux boots Linux
 *
 * Copyright (C) 2003-2005  Eric Biederman (ebiederm@xmission.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

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
#include "../../kexec.h"
#include "../../kexec-elf.h"
#include "kexec-x86.h"
#include "x86-linux-setup.h"
#include "bzImage-loader.h"

int do_bzImage_load(struct kexec_info *info,
	const char *kernel, off_t kernel_len,
	const char *command_line, off_t command_line_len,
	const char *initrd, off_t initrd_len,
	int real_mode_entry, int debug)
{
	struct x86_linux_header setup_header;
	struct x86_linux_param_header *real_mode;
	int setup_sects;
	char *kernel_version;
	size_t size;
	int kern16_size;
	unsigned long setup_base, setup_size;
	struct entry32_regs regs32;
	struct entry16_regs regs16;

	/*
	 * Find out about the file I am about to load.
	 */
	if (kernel_len < sizeof(setup_header)) {
		return -1;
	}
	memcpy(&setup_header, kernel, sizeof(setup_header));
	setup_sects = setup_header.setup_sects;
	if (setup_sects == 0) {
		setup_sects = 4;
	}
	kern16_size = (setup_sects +1) *512;
	kernel_version = ((unsigned char *)&setup_header) + 512 + setup_header.kver_addr;
	if (kernel_len < kern16_size) {
		fprintf(stderr, "BzImage truncated?\n");
		return -1;
	}

	/* Load the trampoline.  This must load at a higher address
	 * the the argument/parameter segment or the kernel will stomp
	 * it's gdt.
	 */
	elf_rel_build_load(info, &info->rhdr, purgatory, purgatory_size,
		0x3000, 640*1024, -1);

	/* The argument/parameter segment */
	setup_size = kern16_size + command_line_len;
	real_mode = xmalloc(setup_size);
	memcpy(real_mode, kernel, kern16_size);
	if (real_mode->protocol_version >= 0x0200) {
		/* Careful setup_base must be greater than 8K */
		setup_base = add_buffer(info, real_mode, setup_size, setup_size,
			16, 0x3000, 640*1024, -1);
	} else {
		add_segment(info, real_mode, setup_size, SETUP_BASE, setup_size);
		setup_base = SETUP_BASE;
	}
	/* Verify purgatory loads higher than the parameters */
	if (info->rhdr.rel_addr < setup_base) {
		die("Could not put setup code above the kernel parameters\n");
	}
	
	/* The main kernel segment */
	size = kernel_len - kern16_size;
	add_segment(info, kernel + kern16_size, size, KERN32_BASE,  size);

		
	/* Tell the kernel what is going on */
	setup_linux_bootloader_parameters(info, real_mode, setup_base,
		kern16_size, command_line, command_line_len,
		initrd, initrd_len);

	/* Get the initial register values */
	elf_rel_get_symbol(&info->rhdr, "entry16_regs", &regs16, sizeof(regs16));
	elf_rel_get_symbol(&info->rhdr, "entry32_regs", &regs32, sizeof(regs32));
	/*

	 * Initialize the 32bit start information.
	 */
	regs32.eax = 0; /* unused */
	regs32.ebx = 0; /* 0 == boot not AP processor start */
	regs32.ecx = 0; /* unused */
	regs32.edx = 0; /* unused */
	regs32.esi = setup_base; /* kernel parameters */
	regs32.edi = 0; /* unused */
	regs32.esp = elf_rel_get_addr(&info->rhdr, "stack_end"); /* stack, unused */
	regs32.ebp = 0; /* unused */
	regs32.eip = KERN32_BASE; /* kernel entry point */

	/*
	 * Initialize the 16bit start information.
	 */
	regs16.cs = setup_base + 0x20;
	regs16.ip = 0;
	regs16.ss = (elf_rel_get_addr(&info->rhdr, "stack_end") - 64*1024) >> 4;
	regs16.esp = 0xFFFC;
	if (real_mode_entry) {
		printf("Starting the kernel in real mode\n");
		regs32.eip = elf_rel_get_addr(&info->rhdr, "entry16");
	}
	if (real_mode && debug) {
		unsigned long entry16_debug, pre32, first32;
		uint32_t old_first32;
		/* Find the location of the symbols */
		entry16_debug = elf_rel_get_addr(&info->rhdr, "entry16_debug");
		pre32 = elf_rel_get_addr(&info->rhdr, "entry16_debug_pre32");
		first32 = elf_rel_get_addr(&info->rhdr, "entry16_debug_first32");
		
		/* Hook all of the linux kernel hooks */
		real_mode->rmode_switch_cs = entry16_debug >> 4;
		real_mode->rmode_switch_ip = pre32 - entry16_debug;
		old_first32 = real_mode->kernel_start;
		real_mode->kernel_start = first32;
		elf_rel_set_symbol(&info->rhdr, "entry16_debug_old_first32",
			&old_first32, sizeof(old_first32));
	
		regs32.eip = entry16_debug;
	}
	elf_rel_set_symbol(&info->rhdr, "entry16_regs", &regs16, sizeof(regs16));
	elf_rel_set_symbol(&info->rhdr, "entry16_debug_regs", &regs16, sizeof(regs16));
	elf_rel_set_symbol(&info->rhdr, "entry32_regs", &regs32, sizeof(regs32));

	/* Fill in the information BIOS calls would normally provide. */
	if (!real_mode_entry) {
		setup_linux_system_parameters(real_mode);
	}

	return 0;
}
	
