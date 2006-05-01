/*
 * kexec: Linux boots Linux
 *
 * Copyright (C) 2006 the funknet.org group (funknet@funknet.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
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
#include <dirent.h>

#include <elf.h>
#include <boot/elf_boot.h>
#include <ip_checksum.h>
#include <x86/x86-linux.h>

#include <monoimage.h>
#include "../../kexec.h"
#include "kexec-x86.h"
#include <arch/options.h>

static const int probe_debug = 0;

int monoimage_probe(const char *buf, off_t len)
{
	struct monoimage_header header;

	fprintf(stderr, "probing for monoimage ...\n");

	if (len < sizeof(header)) {
		return -1;
	}
	memcpy(&header, buf, sizeof(header));
	if (memcmp(header.magic, MI_MAGIC, 2) != 0) {
		if (probe_debug) {
			fprintf(stderr, "probe: not a monoimage\n");
		}
		return -1;
	}
	if (header.version != 0x1) {
		/* Must be version 1 */
		return 0;
	}
	/* I've got a monoimage */
	if (probe_debug) {
		fprintf(stderr, "got monoimage\n");
	}
	return 1;
}


void monoimage_usage(void)
{
	printf(	"-d, --debug		   Enable debugging to help spot a failure.\n"
		"    --real-mode	   Use the kernels real mode entry point.\n"
		"    --command-line=STRING Set the kernel command line to STRING.\n"
		"    --config=STRING	   Set the config file to STRING.\n"
		);
       
}

int monoimage_load(int argc, char **argv, const char *buf, off_t len, 
		   struct kexec_info *info)
{
	char command_line[256];
	char *command_line_append;
	char *configfile;
	char *ramdisk_buf;
	off_t ramdisk_len;
        char *kernel_buf;
	off_t kernel_len;
	int command_line_len;
	int debug, real_mode_entry;
	int opt;
	struct monoimage_header mi_header;
	FILE *image;
	char *image_ptr;
	struct stat config_buf;
	struct stat images_buf;
	struct stat devbuf;
	DIR *dir;
	struct dirent *dev;
	char images_dev[256];
	char config_dev[256];
	FILE *mtab;
	char *buf_ptr;
	char mpoint[256];
	char *mp_ptr;
	char *tmpbuf;
	int fileind;
	int result;

	tmpbuf = (char *)malloc(256 * sizeof(char));

#define OPT_APPEND	OPT_ARCH_MAX+0
#define OPT_CONFIG	OPT_ARCH_MAX+1
#define OPT_REAL_MODE	OPT_ARCH_MAX+2

	static const struct option options[] = {
		KEXEC_ARCH_OPTIONS
		{ "debug",		0, 0, OPT_DEBUG },
		{ "command-line",	1, 0, OPT_APPEND },
		{ "append",		1, 0, OPT_APPEND },
		{ "config",		1, 0, OPT_CONFIG },
		{ "real-mode",		0, 0, OPT_REAL_MODE },
		{ 0,			0, 0, 0 },
	};
	static const char short_options[] = KEXEC_ARCH_OPT_STR "d";

	/*
	 * Parse the command line arguments
	 */
	debug = 0;
	real_mode_entry = 0;
	command_line[0] = '\0';
	command_line_append = 0;
	configfile = 0;
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
		case OPT_REAL_MODE:
			real_mode_entry = 1;
			break;
		case OPT_APPEND:
			command_line_append = optarg;
			break;
		case OPT_CONFIG:
			configfile = optarg;
			break;
		}
	}

	/* configfile is mandatory */
	if (!configfile) {
		fprintf(stderr, "config is required\n");
		return -1;
	} 

	/*
	 * work out which devices we should mount for /images and /config
	 */

	/* our image is in argv, but we don't know which; stat them all to find it */

	for (fileind = 1; fileind < argc; fileind++) { /* don't bother with argv[0]... */
		if (stat(argv[fileind], &images_buf) < 0) {
			if (errno != ENOENT) {
				fprintf(stderr, "stat %s: %s\n", argv[fileind], strerror(errno));
				return -1;
			} 
			else {
				/* ENOENT is expected, press on */
				if (debug) {
					fprintf(stderr, "stat %s: not image\n", argv[fileind]);
				}
			}
		}
		else {
			break;
		}

		if (!images_buf.st_dev) {
			fprintf(stderr, "failed to locate image in argv\n");
			return -1;
		}
	}
	if ((image = fopen(argv[fileind], "r")) < 0) {
		fprintf(stderr, "fopen %s: %s\n",
			argv[fileind],
			strerror(errno));
		return -1;
	}


	/* config file; just stat the known location */

	if (stat(configfile, &config_buf) < 0) {
		fprintf(stderr, "stat %s: %s\n",
			configfile,
			strerror(errno));
		return -1;
	}

	/* walk /dev looking for the block devices that correspond to
	   the files we just statted */

	dir = opendir("/dev");
	if (dir == NULL) {
		fprintf(stderr, "opendir /dev: %s\n",
			strerror(errno));
		return -1;
	}
	while ( ( dev = readdir(dir) ) ) {
		strncpy(tmpbuf, "/dev/", 6);
		strncat(tmpbuf, dev->d_name, strlen(dev->d_name));
		if (stat(tmpbuf, &devbuf) < 0) {
			if (errno != ENOENT) {
				fprintf(stderr, "stat %s: %s\n", dev->d_name, strerror(errno));
				closedir(dir);
				return -1;
			} else {
				/* ENOENT isn't fatal */
				fprintf(stderr, "stat /dev/%s (skipped)\n", dev->d_name);
			}
		}
		if (devbuf.st_rdev == images_buf.st_dev && S_ISBLK(devbuf.st_mode)) {
			/* this is our images device */
			strcpy(images_dev, dev->d_name);
		}
		if (devbuf.st_rdev == config_buf.st_dev && S_ISBLK(devbuf.st_mode)) {
			/* this is our config device */
			strcpy(config_dev, dev->d_name);
		}
	}
	closedir(dir);
	fprintf(stderr, "images device: %s\n", images_dev);
	fprintf(stderr, "config device: %s\n", config_dev);

	/* figure out what the path to the image will be when we mount the images device on /images */
	
	/* read /etc/mtab to find the mount point of images_dev */
	if ( (mtab = fopen("/proc/mounts", "r")) < 0 ) {
		fprintf(stderr, "/proc/mounts: open: %s\n",
			strerror(errno));
		return -1;
	}
	while ( fgets(tmpbuf, 256, mtab) != NULL ) {
		tmpbuf += 5;
		if (strncmp(tmpbuf, images_dev, strlen(images_dev)) == 0) {

			/* skip past the device, the space and the leading  / */
			tmpbuf += (strlen(images_dev) + 1);

			/* find the end of the mountpoint */
			buf_ptr = tmpbuf;
			while (strncmp(buf_ptr, " ", 1) != 0) {
				buf_ptr++;
			}
			strncpy(mpoint, tmpbuf, (buf_ptr - tmpbuf));
			break;
		}
	}

	/* find the common part of the mount point and image path */
	image_ptr = argv[fileind];
	mp_ptr = mpoint;
	while (*mp_ptr == *image_ptr) {
		image_ptr++;
		mp_ptr++;
	}
	/* skip any leading / */
	if (strncmp(image_ptr, "/", 1) == 0 ) {
		image_ptr++;
	}
	
	if (command_line_append) {
		sprintf(command_line, "root=/dev/loop0 ro %s IMAGE=%s IDEV=%s CDEV=%s", 
			command_line_append, 
			image_ptr, images_dev, config_dev);
	} else {
		sprintf(command_line, "root=/dev/loop0 ro IMAGE=%s IDEV=%s CDEV=%s", 
			image_ptr, images_dev, config_dev);
	}
		
	fprintf(stderr, "%s\n", command_line);
	command_line_len = 0;
	if (command_line) {
		command_line_len = strlen(command_line) +1;
	}
	
	/* 
	 * get the monoimage_header, ramdisk and kernel from buf
	 */
	memcpy(&mi_header, buf, sizeof(mi_header));

        if (debug) {
          fprintf(stderr, "monoimage version: %d\n", mi_header.version);
          fprintf(stderr, "kernel at: %ld\n", mi_header.kernel_offset);
          fprintf(stderr, "ramdisk at: %ld\n", mi_header.ramdisk_offset);
          fprintf(stderr, "rootfs at: %ld\n", mi_header.rootfs_offset);
        }

	ramdisk_len = mi_header.rootfs_offset - mi_header.ramdisk_offset;
	ramdisk_buf = malloc(ramdisk_len);
        memcpy(ramdisk_buf, (buf + mi_header.ramdisk_offset), ramdisk_len);

	kernel_len = mi_header.ramdisk_offset - mi_header.kernel_offset;
        kernel_buf = malloc(kernel_len);
	memcpy(kernel_buf, (buf + mi_header.kernel_offset), kernel_len);

	/*
	 * kick off the load of the bzImage+ramdisk 
	 */

	result = do_bzImage_load(info,
				 kernel_buf, kernel_len,
				 command_line, command_line_len,
				 ramdisk_buf, ramdisk_len,
				 real_mode_entry, debug);
	
	return result;
}
