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

/*
 * XXX: exit(3) is nfg in this scenario
 *      path the config file into the rw filesys
 *	logic for third image
 *	datestamp the config upon write
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
#include "monoboot.h"
#include "monoboot_cmds.h"
#include "monoboot_cli.h"

int main(int argc, char **argv) {
    cfg_t *cfg;
    int interact = 1;

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

    cfg = load_config(MB_CONF);
    check_last(cfg);

    if (interact == 1) {
	MB_DEBUG("[mb] main: starting interactive cmdline\n");
	mb_interact(cfg);
    } else {
	MB_DEBUG("[mb] main: booting image\n");
	cmd_boot(cfg, NULL);
    }
    return 0;
}

/* invoke confuse, parse file */
cfg_t* load_config(char *file) {
    cfg_t *cfg;
    int ret;

    static cfg_opt_t image_opts[] = {
	CFG_STR("filename", 0, CFGF_NONE),
	CFG_END()
    };

    static cfg_opt_t opts[] = {
	CFG_INT("version",    0,      CFGF_NONE),
	CFG_STR("default",    "none", CFGF_NONE),
	CFG_STR("bootonce",   "none", CFGF_NONE),
	CFG_STR("fallback",   "none", CFGF_NONE),
	CFG_STR("lastboot",   "none", CFGF_NONE),
	CFG_STR("lasttry",    "none", CFGF_NONE),
	CFG_STR("tryonce",    "none", CFGF_NONE),
	CFG_STR("bootimage",  "none", CFGF_NONE),
	CFG_SEC("image", image_opts, CFGF_MULTI | CFGF_TITLE),
	CFG_END()
    };

    cfg = cfg_init(opts, CFGF_NOCASE);
    ret = cfg_parse(cfg, MB_CONF);
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
int check_last(cfg_t *cfg) {

    /* first thing - unset bootonce flag. */
    cfg_setstr(cfg, "tryonce", "no");

    /* check for success of last boot */
    if (strcmp(cfg_getstr(cfg, "lasttry"), cfg_getstr(cfg, "lastboot")) != 0) {
    	MB_DEBUG("[mb] check_last: last image attempted failed.\n");
	/* the last attempt failed, amend the config to something sane */
	
	if (strcmp(cfg_getstr(cfg, "lasttry"), cfg_getstr(cfg, "default")) == 0) {
	    /* we just tried our default image and it failed. set bootimage to
	       fallback */
	    cfg_setstr(cfg, "bootimage", cfg_getstr(cfg, "fallback"));
	}

	if (strcmp(cfg_getstr(cfg, "lasttry"), cfg_getstr(cfg, "fallback")) == 0) {
	    /* we just tried our fallback image and it failed. yikes.
	       don't do anything, sit here and wait for someone to
	       help us out. */

	    /* XXX */
	}

	return 0;
	
    } else {
    	MB_DEBUG("[mb] check_last: last image attempted succeeded.\n");
	/* it worked. if we just booted fallback properly, switch back
	   to default. otherwise leave alone - we've already switched
	   off bootonce, so we'll boot default as per normal */
	
	if (strcmp(cfg_getstr(cfg, "lasttry"), cfg_getstr(cfg, "fallback")) == 0) {
	    cfg_setstr(cfg, "bootimage", cfg_getstr(cfg, "default"));
	}

	return 1;
    }
}

/* write config back to file, with shimmying */
void save_config(cfg_t *cfg) {
    FILE *fp = fopen(MB_CONF_NEW, "w");

    MB_DEBUG("[mb] save_config: writing config to %s\n", MB_CONF_NEW);
    if (!fp) {
	perror(MB_CONF_NEW); 
	exit(3);
    }
    cfg_print(cfg, fp); /* XXX check this for errors */
    fclose(fp);

    MB_DEBUG("[mb] save_config: preserving old config as %s\n", MB_CONF_OLD);
    if (rename(MB_CONF, MB_CONF_OLD)) {
	perror(MB_CONF_OLD);
	exit(4);
    }
    MB_DEBUG("[mb] save_config: putting new config in place\n");
    if (rename(MB_CONF_NEW, MB_CONF)) {
	perror(MB_CONF);
	exit(5);
    }
    return;
}

/* clean up */
void drop_config(cfg_t *cfg) {
    cfg_free(cfg);
}

  
