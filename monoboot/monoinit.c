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
  cmd_write(NULL, cfg, NULL);
}

/* clean up */
void drop_config(cfg_t *cfg) 
{
    cfg_free(cfg);
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

int parse_cmdline(char *imagefile, 
		  char *configdev,
		  char *configpath
		  )
{
    FILE *file;
    char *value;
    char cmdline[MB_CMDLINE_MAX];
    
    /* open /proc/cmdline, read image filename */
    if ( (file = fopen("/proc/cmdline", "r")) == NULL ) {
	fprintf(stderr, "/proc/cmdline: open: %s\n",
		strerror(errno));
	return MB_CMDLINE_NOPROC;
    }
    if ( fgets(cmdline, MB_CMDLINE_MAX, file) == NULL ) {
	fprintf(stderr, "read /proc/cmdline: %s\n",
		strerror(errno));
	return MB_CMDLINE_NOPROC;
    }
    fclose(file);    

    /* parse out actual image filename and images device */
    if ((value = get_cmdline_var(cmdline, "IMAGE")) != NULL) {
	strcpy(imagefile, value);
    } else {
	fprintf(stderr, "no IMAGE in cmdline\n");
	return MB_CMDLINE_PARSE;
    } 

    if ((value = get_cmdline_var(cmdline, "CONFIG")) != NULL) {
	strcpy(configpath, value);
    } else {
	fprintf(stderr, "no CONFIG in cmdline\n");
	return MB_CMDLINE_PARSE;
    } 
    
    if ((value = get_cmdline_var(cmdline, "CDEV")) != NULL) {
	strcpy(configdev, "/dev/");
	strcat(configdev, value);
    } else {
	fprintf(stderr, "no CDEV in cmdline\n");
	return MB_CMDLINE_PARSE;
    } 

    if (strlen(imagefile) && strlen(configdev)) {
	return 0;
    } else {
	return MB_CMDLINE_PARSE;
    }
}

int main (void) 
{
    cfg_t *cfg;
    char image[MB_PATH_MAX];
    char cdev[MB_PATH_MAX];
    char cpath[MB_PATH_MAX];
    char rsync_cmd[MB_CMDLINE_MAX];
    char *filename;
    int images, n;

    /* find the config dev from the kernel cmdline */
    if ((n = parse_cmdline(image, cdev, cpath)) != 0) {
	if (n == MB_CMDLINE_NOPROC) {
	    fprintf(stderr, "[mi] couldn't read from /proc/cmdline\n");
	    exit(1);
	} else if (n == MB_CMDLINE_PARSE) {
	    fprintf(stderr, "[mi] failed to parse cmdline\n");
	    exit(1);
	} else {
	    fprintf(stderr, "[mi] unknown error finding config dev, path / image\n");
	    exit(1);
	}
    }
    fprintf(stderr, "[mi] config dev:  %s\n", cdev);
    fprintf(stderr, "[mi] config path: %s\n", cpath);
    fprintf(stderr, "[mi] image file:  %s\n", image);
    
    /* mount cdev on /config */
    if ( mount (cdev, "/config", "ext3", 0, NULL) < 0 ) {
	fprintf(stderr, "mount /config failed: %s\n",
		strerror(errno));
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

    /* mount a ramfs on /etc */
    if (do_exec(MOUNT_BINARY, "mount", "-t", "tmpfs", "tmpfs", "/etc", 0) != 0) {
	MB_DEBUG("[mb] mount /etc failed: %s\n",
		 strerror(errno));
    }

    /* clone cpath's contents into /etc */
    sprintf(rsync_cmd, "%s -rt %s/etc/ /etc/", RSYNC_BINARY, cpath);
    MB_DEBUG("[mb] will system cmdline: %s\n", rsync_cmd);
    system(rsync_cmd);

    /* done */
    exit(0);
}
