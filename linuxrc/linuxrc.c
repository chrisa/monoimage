#define _GNU_SOURCE
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>   
#include <sys/reboot.h>
#include "monoimage.h"
#include "loop.h"

#define IMAGES_DEV "/dev/hda1"
#define FS_DEV "/dev/hda1"
#define FS_IMG "failsafe.img"

/*
 *
 * linuxrc.c - monoimage support
 * 
 * $Author$
 * $Revision$
 *
 */

static int loop_info64_to_old(const struct loop_info64 *info64, struct loop_info *info)
{
        memset(info, 0, sizeof(*info));
        info->lo_number = info64->lo_number;
        info->lo_device = info64->lo_device;
        info->lo_inode = info64->lo_inode;
        info->lo_rdevice = info64->lo_rdevice;
        info->lo_offset = info64->lo_offset;
        info->lo_encrypt_type = info64->lo_encrypt_type;
        info->lo_encrypt_key_size = info64->lo_encrypt_key_size;
        info->lo_flags = info64->lo_flags;
        info->lo_init[0] = info64->lo_init[0];
        info->lo_init[1] = info64->lo_init[1];
        memcpy(info->lo_name, info64->lo_file_name, LO_NAME_SIZE);
        memcpy(info->lo_encrypt_key, info64->lo_encrypt_key, LO_KEY_SIZE);

        /* error in case values were truncated */
        if (info->lo_device != info64->lo_device ||
            info->lo_rdevice != info64->lo_rdevice ||
            info->lo_inode != info64->lo_inode ||
            info->lo_offset != info64->lo_offset)
                return -EOVERFLOW;

        return 0;
}


int set_loop(const char *device, const char *file, int offset)
{
        struct loop_info64 loopinfo64;
        int fd, ffd;

        if ((ffd = open(file, O_RDWR)) < 0) {
		fprintf(stderr, "%s: open: %s\n",
			file,
			strerror(errno));
		return 1;
        }
        if ((fd = open(device, O_RDWR)) < 0) {
		fprintf(stderr, "%s: open: %s\n",
			device,
			strerror(errno));
                return 1;
        }

        memset(&loopinfo64, 0, sizeof(loopinfo64));
        strncpy(loopinfo64.lo_file_name, file, LO_NAME_SIZE);
        loopinfo64.lo_offset = offset;
        loopinfo64.lo_encrypt_key_size = 0;

        if (ioctl(fd, LOOP_SET_FD, ffd) < 0) {
                fprintf(stderr, "ioctl: LOOP_SET_FD\n");
                return 1;
        }
        if (ioctl(fd, LOOP_SET_STATUS64, &loopinfo64) < 0) {
                struct loop_info loopinfo;
                int errsv = errno;

                errno = loop_info64_to_old(&loopinfo64, &loopinfo);
                if (errno) {
                        errno = errsv;
                        fprintf(stderr, "ioctl: LOOP_SET_STATUS64\n");
                        goto fail;
                }

                if (ioctl(fd, LOOP_SET_STATUS, &loopinfo) < 0) {
                        fprintf(stderr, "ioctl: LOOP_SET_STATUS\n");
                        goto fail;
                }
        }

        close (ffd);
        close (fd);
        return 0;

 fail:
        (void) ioctl (fd, LOOP_CLR_FD, 0);
        close (ffd);
        close (fd);
        return 1;
}

void die_reboot (void)
{
	sleep(12);
	fprintf(stderr, "rebooting...");
	if (reboot(RB_AUTOBOOT) != 0 ) {
		fprintf(stderr, "reboot failed: %s\n", 
			strerror(errno));
		exit(1);
	}
}

void die_halt (void)
{
	fprintf(stderr, "halting system.");
	if (reboot(RB_HALT_SYSTEM) != 0 ) {
		fprintf(stderr, "halt failed: %s\n", 
			strerror(errno));
		exit(1);
	}
}

int main (int argc, char **argv)
{
	FILE *file;
	char *cmdline;
	char *cmdline_ptr;
	char *imagefile;
	char *imagedev;
	int offset;
	struct monoimage_header bi_header;
	struct stat s;

	fprintf(stderr, "monoimage linuxrc starting...\n");

	/* buffers */
	cmdline   = (char *)malloc(256 * sizeof(char));
	imagefile = (char *)malloc(256 * sizeof(char));
	imagedev  = (char *)malloc(256 * sizeof(char));

	/* mount proc */
	if ( mount("proc", "/proc", "proc", 0, NULL) < 0 ) {
		fprintf(stderr, "mount /proc: %s\n",
			strerror(errno));
		die_reboot();
	}

	/* open /proc/cmdline, read image filename */
	if ( (file = fopen("/proc/cmdline", "r")) < 0 ) {
		fprintf(stderr, "/proc/cmdline: open: %s\n",
			strerror(errno));
		die_reboot();
	}
	if ( fgets(cmdline, 256, file) == NULL ) {
		fprintf(stderr, "read /proc/cmdline: %s\n",
			strerror(errno));
		die_reboot();
	}

	/* parse out actual image filename and images device */
	while ( strncmp(cmdline, "\n", 1) != 0 ) {

		if ( strncmp(cmdline, "IMAGE=", 6) == 0 ) {

			cmdline_ptr = cmdline;
			cmdline += 6; /* skip past IMAGE= */
			while ( strncmp(cmdline_ptr, " ", 1) != 0 && strncmp(cmdline_ptr, "\n", 1) != 0) {
				cmdline_ptr++;
			}
			strncpy(imagefile, "/images/", 9);
			strncat(imagefile, cmdline, (cmdline_ptr - cmdline));
		}

		if ( strncmp(cmdline, "DEV=", 4) == 0 ) {

			cmdline_ptr = cmdline;
			cmdline += 4; /* skip past DEV= */
			while ( strncmp(cmdline_ptr, " ", 1) != 0 && strncmp(cmdline_ptr, "\n", 1) != 0) {
				cmdline_ptr++;
			}
			strncpy(imagedev, "/dev/", 6);
			strncat(imagedev, cmdline, (cmdline_ptr - cmdline));
		}

		cmdline++;
	}

	fclose(file);

	fprintf(stderr, "image file: %s\n", imagefile);
	fprintf(stderr, "image dev:  %s\n", imagedev);

	/* mount imagedev on /images */
	if ( mount (imagedev, "/images", "ext3", 0, NULL) < 0 ) {
		fprintf(stderr, "mount /images: %s\n",
			strerror(errno));
		die_reboot();
	}

	/* check we can find the image file */
	if ( stat (imagefile, &s) != 0 ) {
		fprintf(stderr, "stat %s failed: %s\n",
			imagefile,
			strerror(errno));
		/* 
		 * try the failsafe image, which 
		 * should be /failsafe.img on FS_DEV
		 * (umount /images first..)
		 */

		strncpy(imagefile, "/images/", 9);
		strcat(imagefile, FS_IMG);

		if ( umount("/images") != 0) {
			fprintf(stderr, "umount /images failed: %s\n",
				strerror(errno));
			die_reboot();
		}
		if ( mount (FS_DEV, "/images", "ext3", 0, NULL) < 0 ) {
			fprintf(stderr, "mount failsafe /images: %s\n",
				strerror(errno));
			die_reboot();
		}
		if ( stat (imagefile, &s) != 0 ) {
			fprintf(stderr, "stat failsafe image %s failed: %s\n",
				imagefile,
				strerror(errno));
			die_reboot();
		}
		fprintf(stderr, "using failsafe image %s\n", imagefile);
	}

	/* read header from /images/%s, get rootfs offset */
	if ( (file = fopen(imagefile, "r")) < 0 ) {
		fprintf(stderr, "%s: open: %s\n",
			imagefile,
			strerror(errno));
		die_reboot();
	}
	if ( fread(&bi_header, sizeof(bi_header), 1, file) != 1 ) {
		fprintf(stderr, "read %s: %s\n",
			imagefile,
			strerror(errno));
		die_reboot();
	}
	offset = bi_header.rootfs_offset;
	
	/* losetup image on /dev/loop0 */
	if( set_loop("/dev/loop0", imagefile, offset) < 0 ) {
		fprintf(stderr, "set_loop: %s\n",
			strerror(errno));
		die_reboot();
	}
	
	/* success! */
	exit(0);
}
