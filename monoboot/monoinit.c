#define C99_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
#include <sys/mount.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <confuse.h>
#include "mbconf.h"
#include "monoboot.h"

/* $Id$ */

/* invoke confuse, parse file */
cfg_t* load_config(char *file) 
{
    cfg_t *cfg;
    int ret;

    cfg = cfg_init(opts, CFGF_NOCASE);
    ret = cfg_parse(cfg, file);
    if (ret == CFG_FILE_ERROR) {
	perror(MB_CONF);
	exit(1);
    } else if (ret == CFG_PARSE_ERROR) {
	fprintf(stderr, "parse error on %s\n",MB_CONF);
	exit(2);
    }
    MB_DEBUG("[mi] load_config: config parsed ok\n");
    return cfg;
}

/* write config back to file, with shimmying */
void save_config(cfg_t *cfg) 
{
    cmd_write(cfg, NULL);
}

/* clean up */
void drop_config(cfg_t *cfg) 
{
    cfg_free(cfg);
}

int parse_cmdline(char *imagefile, char *configdev)
{
    FILE *file;
    char *cmdline;
    char *cmdline_ptr;

    /* open /proc/cmdline, read image filename */
    if ( (file = fopen("/proc/cmdline", "r")) == NULL ) {
	fprintf(stderr, "/proc/cmdline: open: %s\n",
		strerror(errno));
	exit(1);
    }
    if ( fgets(cmdline, 256, file) == NULL ) {
	fprintf(stderr, "read /proc/cmdline: %s\n",
		strerror(errno));
	exit(1);
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
	
	if ( strncmp(cmdline, "CDEV=", 5) == 0 ) {
	    
	    cmdline_ptr = cmdline;
	    cmdline += 5; /* skip past DEV= */
	    while ( strncmp(cmdline_ptr, " ", 1) != 0 && strncmp(cmdline_ptr, "\n", 1) != 0) {
		cmdline_ptr++;
	    }
	    strncpy(configdev, "/dev/", 6);
	    strncat(configdev, cmdline, (cmdline_ptr - cmdline));
	}
	
	cmdline++;
    }
    fclose(file);
    return 0;
}

int main (void) 
{
    cfg_t *cfg;
    char cdev[256];
    char image[256];
    char *filename;
    int images, n;

    /* find the config dev from the kernel cmdline */
    if (parse_cmdline(image, cdev) != 0) {
	fprintf(stderr, "[mi] failed to parse cmdline - is /proc/mounted?");
	exit(1);
    }
    fprintf(stderr, "[mi] config dev: %s\n", cdev);
    fprintf(stderr, "[mi] image file: %s\n", image);
    
    /* mount cdev on /config */
    if ( mount (cdev, "/config", "ext3", 0, NULL) < 0 ) {
	fprintf(stderr, "mount /config: %s\n",
		strerror(errno));
	exit(1);
    }
    
    /* load the mb.conf */
    cfg = load_config("/config/mb.conf");
    
    /* iterate images to find ours */
    images = cfg_size(cfg, "image");
    for (n = 0; n < images; n++) {
	cfg_t *image_sec = cfg_getnsec(cfg, "image", n);
	filename = cfg_getstr(image_sec, "filename");
	if (strncmp(filename, image, strlen(image)) == 0) {
	    /* this is us, set lastboot */
	    cfg_setstr(cfg, "lastboot", cfg_title(image_sec));
	    break;
	}
    }

    /* write back */
    save_config(cfg);
    drop_config(cfg);
    
    /* done */
    exit(0);
}
