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
#include <arch/options.h>

static const int probe_debug = 0;

int bzImage_probe(const char *buf, off_t len)
{
	struct x86_linux_header header;
	if (len < sizeof(header)) {
		return -1;
	}
	memcpy(&header, buf, sizeof(header));
	if (memcmp(header.header_magic, "HdrS", 4) != 0) {
		if (probe_debug) {
			fprintf(stderr, "Not a bzImage\n");
		}
		return -1;
	}
	if (header.boot_sector_magic != 0xAA55) {
		if (probe_debug) {
			fprintf(stderr, "No x86 boot sector present\n");
		}
		/* No x86 boot sector present */
		return -1;
	}
	if (header.protocol_version < 0x0200) {
		if (probe_debug) {
			fprintf(stderr, "Must be at least protocol version 2.00\n");
		}
		/* Must be at least protocol version 2.00 */
		return -1;
	}
	if ((header.loadflags & 1) == 0) {
		if (probe_debug) {
			fprintf(stderr, "zImage not a bzImage\n");
		}
		/* Not a bzImage */
		return -1;
	}
	/* I've got a bzImage */
	if (probe_debug) {
		fprintf(stderr, "It's a bzImage\n");
	}
	return 0;
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

int bzImage_load(int argc, char **argv, const char *buf, off_t len, 
	struct kexec_info *info)
{
	const char *command_line;
	const char *ramdisk;
	char *ramdisk_buf;
	off_t ramdisk_length;
	int command_line_len;
	int debug, real_mode_entry;
	int opt;
	int result;
#define OPT_APPEND	(OPT_ARCH_MAX+0)
#define OPT_RAMDISK	(OPT_ARCH_MAX+1)
#define OPT_REAL_MODE	(OPT_ARCH_MAX+2)
	static const struct option options[] = {
		KEXEC_ARCH_OPTIONS
		{ "debug",		0, 0, OPT_DEBUG },
		{ "command-line",	1, 0, OPT_APPEND },
		{ "append",		1, 0, OPT_APPEND },
		{ "initrd",		1, 0, OPT_RAMDISK },
		{ "ramdisk",		1, 0, OPT_RAMDISK },
		{ "real-mode",		0, 0, OPT_REAL_MODE },
		{ 0, 			0, 0, 0 },
	};
	static const char short_options[] = KEXEC_ARCH_OPT_STR "d";

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
			if (opt < OPT_ARCH_MAX) {
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
	ramdisk_buf = 0;
	if (ramdisk) {
		ramdisk_buf = slurp_file(ramdisk, &ramdisk_length);
	}
	result = do_bzImage_load(info,
		buf, len,
		command_line, command_line_len,
		ramdisk_buf, ramdisk_length,
		real_mode_entry, debug);

	return result;
}
