#define _GNU_SOURCE
#define _C99_SOURCE
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
#include <linux/loop.h>
#include "monoimage.h"

#define IMAGES_DEV "/dev/hda1"
#define FS_DEV "/dev/hda1"
#define FS_IMG "failsafe.img"
#define BUF_MAX 256

/*
 *
 * linuxrc.c - monoimage support
 * 
 * $Author$
 * $Revision$
 *
 */

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
        strncpy((char *)loopinfo64.lo_file_name, file, LO_NAME_SIZE);
        loopinfo64.lo_offset = offset;
        loopinfo64.lo_encrypt_key_size = 0;

        if (ioctl(fd, LOOP_SET_FD, ffd) < 0) {
                fprintf(stderr, "ioctl: LOOP_SET_FD\n");
                return 1;
        }
        if (ioctl(fd, LOOP_SET_STATUS64, &loopinfo64) < 0) {
                fprintf(stderr, "ioctl: LOOP_SET_STATUS64\n");
                goto fail;
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

static char *get_cmdline_var (char *cmdline, char *var) {
    char *start_ptr, *end_ptr;
    char *value;
    
    start_ptr= strstr(cmdline, var);
    if (start_ptr) {
	end_ptr = strstr(start_ptr, " ");
	if (end_ptr == '\0') {
	    end_ptr = strstr(start_ptr, "\n");
	}
	start_ptr += (strlen(var) + 1); /* +1 for the =, not a NULL */
	value = strndup(start_ptr, (end_ptr - start_ptr));
    } else {
	return NULL;
    }
    return value;
}

int main (int argc, char **argv)
{
	FILE *file;
	char imagefile[BUF_MAX];
	char configdev[BUF_MAX];
	char imagedev[BUF_MAX];
	char cmdline[BUF_MAX];
	char *value;
	int offset;
	struct monoimage_header bi_header;
	struct stat s;

	fprintf(stderr, "monoimage linuxrc starting...\n");

	/* mount proc */
	if ( mount("proc", "/proc", "proc", 0, NULL) < 0 ) {
		fprintf(stderr, "mount /proc: %s\n",
			strerror(errno));
		die_reboot();
	}

	/* open /proc/cmdline, read image filename */
	if ( (file = fopen("/proc/cmdline", "r")) == NULL ) {
		fprintf(stderr, "/proc/cmdline: open: %s\n",
			strerror(errno));
		die_reboot();
	}
	if ( fgets(cmdline, BUF_MAX, file) == NULL ) {
		fprintf(stderr, "read /proc/cmdline: %s\n",
			strerror(errno));
		die_reboot();
	}

	fclose(file);

	/* parse out actual image filename and image/config devices */
	if ((value = get_cmdline_var(cmdline, "IMAGE")) != NULL) {
	    strcpy(imagefile, "/images/");
	    strcat(imagefile, value);
	} else {
	    fprintf(stderr, "no IMAGE in cmdline\n");
	    die_reboot();
	} 

	if ((value = get_cmdline_var(cmdline, "IDEV")) != NULL) {
	    strcpy(imagedev, "/dev/");
	    strcat(imagedev, value);
	} else {
	    fprintf(stderr, "no IDEV in cmdline\n");
	    die_reboot();
	} 

	if ((value = get_cmdline_var(cmdline, "CDEV")) != NULL) {
	    strcpy(configdev, "/dev/");
	    strcat(configdev, value);
	} else {
	    fprintf(stderr, "no CDEV in cmdline\n");
	    die_reboot();
	} 
	    
	fprintf(stderr, "image file: %s\n", imagefile);
	fprintf(stderr, "image dev:  %s\n", imagedev);
	fprintf(stderr, "config dev: %s\n", configdev);

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

		strcpy(imagefile, "/images/");
		strcat(imagefile, FS_IMG);

		if ( umount2("/images",0) != 0) {
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
	if ( (file = fopen(imagefile, "r")) == NULL ) {
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
