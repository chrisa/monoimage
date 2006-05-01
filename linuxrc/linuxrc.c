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

#include <dirent.h>
#include <unistd.h>   
#include <sys/reboot.h>
#include <linux/loop.h>
#include "monoimage.h"
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2_types.h>
#include <ext2fs/ext2fs.h>

#define IMAGES_DEV "/dev/hda1"
#define FS_DEV "/dev/hda1"
#define FS_IMG "failsafe.img"
#define BUF_MAX 256

/* for debugging don't reboot the machine */
#ifdef DEBUG
# define DIE die_halt();
#else
# define DIE die_reboot();
#endif

struct uuid {
	__u32	time_low;
	__u16	time_mid;
	__u16	time_hi_and_version;
	__u16	clock_seq;
	__u8	node[6];
};

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
                
/* Returns 1 if the uuid is the NULL uuid */
int is_null_uuid(void *uu)
{
	__u8 	*cp;
	int	i;

	for (i=0, cp = uu; i < 16; i++)
		if (*cp)
			return 0;
	return 1;
}

static void unpack_uuid(void *in, struct uuid *uu)
{
	__u8	*ptr = in;
	__u32	tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	tmp = (tmp << 8) | *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_low = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_mid = tmp;
	
	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_hi_and_version = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->clock_seq = tmp;

	memcpy(uu->node, ptr, 6);
}

void uuid_to_str(void *uu, char *out)
{
	struct uuid uuid;

	unpack_uuid(uu, &uuid);
	sprintf(out,
		"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		uuid.time_low, uuid.time_mid, uuid.time_hi_and_version,
		uuid.clock_seq >> 8, uuid.clock_seq & 0xFF,
		uuid.node[0], uuid.node[1], uuid.node[2],
		uuid.node[3], uuid.node[4], uuid.node[5]);
}

const char *uuid2str(void *uu)
{
	static char buf[80];

	if (is_null_uuid(uu))
		return "<none>";
	uuid_to_str(uu, buf);
	return buf;
}

const char *ext2_uuid(char *device)
{
        errcode_t e2fserr;
        ext2_filsys fs;
        const char *uuid;
        
        e2fserr = ext2fs_open(device, 0, 0, 0, unix_io_manager, &fs);
        if (e2fserr) {
                uuid = "<none>";
        }
        else {
                uuid = uuid2str(fs->super->s_uuid);
                e2fserr = ext2fs_close(fs);
        }

        return uuid;
}

int main (int argc, char **argv)
{
	FILE *file;
        DIR *dir;
	struct dirent *dev;
	char imagefile[BUF_MAX];
	char configuuid[BUF_MAX];
	char imageuuid[BUF_MAX];
	char cmdline[BUF_MAX];
        char devname[BUF_MAX];
        char uuid[BUF_MAX];
	char *value;
	off_t offset;
	struct monoimage_header bi_header;
	struct stat s;

	fprintf(stderr, "MI: linuxrc starting...\n");

	/* mount proc */
	if ( mount("proc", "/proc", "proc", 0, NULL) < 0 ) {
		fprintf(stderr, "mount /proc: %s\n",
			strerror(errno));
		DIE;
	}

	/* open /proc/cmdline, read image filename */
	if ( (file = fopen("/proc/cmdline", "r")) == NULL ) {
		fprintf(stderr, "/proc/cmdline: open: %s\n",
			strerror(errno));
		DIE;
	}
	if ( fgets(cmdline, BUF_MAX, file) == NULL ) {
		fprintf(stderr, "read /proc/cmdline: %s\n",
			strerror(errno));
		DIE;
	}

	fclose(file);

	/* parse out actual image filename and image/config devices */
	if ((value = get_cmdline_var(cmdline, "F")) != NULL) {
                strcpy(imagefile, "/images/");
                strcat(imagefile, value);
	} else {
                fprintf(stderr, "no F in cmdline\n");
                DIE;
	} 

	if ((value = get_cmdline_var(cmdline, "I")) != NULL) {
                strcpy(imageuuid, value);
	} else {
                fprintf(stderr, "no I in cmdline\n");
                DIE;
	} 

	if ((value = get_cmdline_var(cmdline, "C")) != NULL) {
                strcpy(configuuid, value);
	} else {
                fprintf(stderr, "no C in cmdline\n");
                DIE;
	} 
	    
	fprintf(stderr, "MI: image file: %s\n", imagefile);
	fprintf(stderr, "MI: image uuid:  %s\n", imageuuid);
	fprintf(stderr, "MI: config uuid: %s\n", configuuid);

        /* walk /dev looking for the uuid we're interested in */
        dir = opendir("/dev");
	if (dir == NULL) {
                fprintf(stderr, "opendir /dev: %s\n",
                        strerror(errno));
                DIE;
	}

	while ( ( dev = readdir(dir) ) ) {

		strncpy(devname, "/dev/", 6);
		strncat(devname, dev->d_name, strlen(dev->d_name));

		if (stat(devname, &s) == 0 && S_ISBLK(s.st_mode)) {
                        strcpy(uuid, ext2_uuid(devname));
                        if (strcmp(imageuuid, uuid) == 0) {
                                /* mount this device on /images */
                                fprintf(stderr, "MI: mounting %s on /images\n", devname);
                                if ( mount (devname, "/images", "ext3", 0, NULL) < 0 ) {
                                        fprintf(stderr, "mount /images: %s\n",
                                                strerror(errno));
                                        DIE;
                                }
                                else {
                                        goto DONE;
                                }
                        }
                }
        }
        
 DONE:

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
                        /* don't die, it might fail to umount 'cos the 
                           previous mount failed */
                }
                if ( mount (FS_DEV, "/images", "ext3", 0, NULL) < 0 ) {
                        fprintf(stderr, "mount failsafe /images: %s\n",
                                strerror(errno));
                        DIE;
                }
                if ( stat (imagefile, &s) != 0 ) {
                        fprintf(stderr, "stat failsafe image %s failed: %s\n",
                                imagefile,
                                strerror(errno));
                        DIE;
                }
                fprintf(stderr, "using failsafe image %s\n", imagefile);
        }

        /* read header from /images/%s, get rootfs offset */
        if ( (file = fopen(imagefile, "r")) == NULL ) {
                fprintf(stderr, "%s: open: %s\n",
                        imagefile,
                        strerror(errno));
                DIE;
        }
        if ( fread(&bi_header, sizeof(bi_header), 1, file) != 1 ) {
                fprintf(stderr, "read %s: %s\n",
                        imagefile,
                        strerror(errno));
                DIE;
        }
        offset = bi_header.rootfs_offset;
        fprintf(stderr, "MI: root filesystem at %ld\n", offset);        
	
        /* losetup image on /dev/loop0 */
        if( set_loop("/dev/loop0", imagefile, offset) < 0 ) {
                fprintf(stderr, "set_loop: %s\n",
                        strerror(errno));
                DIE;
        }
	
        /* success! */
        exit(0);
}
