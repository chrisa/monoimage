/*
 * monoboot
 *
 * chooses a monoimage to boot. 
 *
 * looks at the boot config file for the preferred image, then at the boot log
 * to decide if we should go for the fallback image. then kexec-load the image,
 * log our decision, make sure we're mounted read-only and then kexec the image
 * 
 */

/* $Id$ */

/* TODO
 *      path the config file into the rw filesys
 *      'show disk' file listing 
 *      tftp files into right place (disk: == /images ?)
 *      remount rw as appropriate
 */

#define C99_SOURCE

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <getopt.h>
#include <confuse.h>
#include <malloc.h>

#include "mbconf.h"
#include "monoboot.h"

cfg_t *cfg;

int main(int argc, char **argv) {
    int interact = 1;
    char conf_path[MB_PATH_MAX];
    char bootimage[MB_IMAGE_MAX];
    int delay = 0;
    char *cmdline[2];
    int ret;

    /* make sure important things are mounted */
    if (check_mounted(MB_PATH_CONFIG) != MB_CM_YES) {
	MB_DEBUG("[mb] mounting config partition\n");
	if (do_exec(MOUNT_BINARY, "mount", MB_PATH_CONFIG, 0) != 0) {
	    MB_DEBUG("[mb] mount config partition failed\n");
	}
    }
    if (check_mounted(MB_PATH_IMAGES) != MB_CM_YES) {
	MB_DEBUG("[mb] mounting images partition\n");
	if (do_exec(MOUNT_BINARY, "mount", MB_PATH_IMAGES, 0) != 0) {
	    MB_DEBUG("[mb] mount images partition failed\n");
	}
    }
    
    /* load config */
    sprintf(conf_path, "%s/%s", MB_PATH_CONFIG, MB_CONF);
    MB_DEBUG("[mb] main: conf file is %s\n", conf_path);
    cfg = load_config(conf_path);

    /* look to see if we're mbnet */
    if (!strncmp(basename(argv[0]),"mbnet",5)) {
	MB_DEBUG("[mb] running as mbnet\n");
	do_netconf(cfg);
	do_exit();
    }

    /* run as either interactive or non-interactive */
    if (!strncmp(basename(argv[0]),"mbsh",4)) {
	MB_DEBUG("[mb] running as mbsh\n");
	interact = 1;
    } else if (!strncmp(basename(argv[0]),"monoboot",8)) {
	MB_DEBUG("[mb] running as monoboot\n");
	interact = 0;
    } else {
	/* unsure what we've been run as */
	MB_DEBUG("[mb] unknown argv0 %s\n", argv[0]);
	interact = 1;
    }

    
    /* if we're not already doing interactive mode, get the delay from
       config , wait for keypress if > 0 */
    if (!interact) {
	delay = cfg_getint(cfg, "delay");
	if (get_keypress(delay) == 0) {
	    interact = 1;
	}
    }

    /* deal with the fallout from last time round */
    ret = check_last(cfg, bootimage);
    if (ret) {
	MB_DEBUG("[mb] main: last boot successful, will boot %s now\n", bootimage);
    } else {
	MB_DEBUG("[mb] main: last boot failed, will boot %s now\n", bootimage);
    }

    if (interact == 1) {
	MB_DEBUG("[mb] main: configuring network\n");
	/* do_netconf(cfg); */
	MB_DEBUG("[mb] main: starting interactive cmdline\n");
	mb_interact(cfg);
    } else {
	MB_DEBUG("[mb] main: booting image\n");
	cmdline[1] = bootimage;
	cmd_boot(NULL, cfg, cmdline);
    }
    return 0;
}

/* invoke confuse, parse file */
cfg_t* load_config(char *file) {
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
    MB_DEBUG("[mb] load_config: config parsed ok\n");
    return cfg;
}

/* examine last boot's success */
int check_last(cfg_t *cfg, char *image) {
    

    /* first thing - unset bootonce flag. */
    cfg_setstr(cfg, "tryonce", "no");
    
    /* check for success of last boot */
    if (strcmp(cfg_getstr(cfg, "lasttry"), cfg_getstr(cfg, "lastboot")) != 0) {
    	MB_DEBUG("[mb] check_last: last image attempted failed.\n");
	/* the last attempt failed, amend the config to something sane */
	
	if (strcmp(cfg_getstr(cfg, "lasttry"), cfg_getstr(cfg, "fallback")) == 0) {
	    MB_DEBUG("[mb] check_last: fallback failed, panicking.\n");
	    /* we just tried our fallback image and it failed. yikes.
	       don't do anything, sit here and wait for someone to
	       help us out. */
	    /* XXX */
	}

	strcpy(image, cfg_getstr(cfg, "fallback"));
	MB_DEBUG("DEBUG: set bootimage to %s\n", image);
	return 0;	

    } else {
    	MB_DEBUG("[mb] check_last: last image attempted succeeded.\n");
	/* it worked. if we just booted fallback properly, switch back
	   to default. otherwise leave alone - we've already switched
	   off bootonce, so we'll boot default as per normal */
	
	strcpy(image, cfg_getstr(cfg, "default"));
	MB_DEBUG("DEBUG: set bootimage to %s\n", image);
	return 1;
    }
}

/* write config back to file, with shimmying */
void save_config(cfg_t *cfg) {
  cmd_write(NULL, cfg, NULL);
}

/* clean up */
void drop_config(cfg_t *cfg) {
    cfg_free(cfg);
}

