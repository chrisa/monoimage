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
#include <signal.h>
#include <setjmp.h>
#include <termios.h>
#include "monoboot.h"

int main(int argc, char **argv) {
    cfg_t *cfg;
    int interact = 1;
    char conf_path[MB_PATH_MAX];
    int delay = 0;

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

    /* get everything mounted from fstab so we can load config */
    if (do_exec(MOUNT_BINARY, "mount", "-a", 0) != 0) {
	MB_DEBUG("[mb] mount -a failed\n");
    }

    sprintf(conf_path, "%s/%s", MB_PATH_CONFIG, MB_CONF);
    MB_DEBUG("[mb] main: conf file is %s\n", conf_path);
    cfg = load_config(conf_path);

    
    /* if we're not already doing interactive mode, get the delay from
       config , wait for keypress if > 0 */
    if (!interact) {
	delay = cfg_getint(cfg, "delay");
	if (get_keypress(delay) == 0) {
	    interact = 1;
	}
    }

    /* deal with the fallout from last time round */
    check_last(cfg);

    if (interact == 1) {
	MB_DEBUG("[mb] main: configuring network\n");
	do_netconf(cfg);
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

    static cfg_opt_t network_opts[] = {
	CFG_STR("address", 0, CFGF_NONE),
	CFG_STR("gateway", 0, CFGF_NONE),
	CFG_END()
    };

    static cfg_opt_t opts[] = {
	CFG_INT("version",    0,            CFGF_NONE),
	CFG_INT("delay",      0,            CFGF_NONE),
	CFG_STR("bootonce",   "none",       CFGF_NONE),
	CFG_STR("tryonce",    "none",       CFGF_NONE),
	CFG_STR("default",    "none",       CFGF_NONE),
	CFG_STR("fallback",   "none",       CFGF_NONE),
	CFG_STR("lasttry",    "none",       CFGF_NONE),
	CFG_STR("lastboot",   "none",       CFGF_NONE),
	CFG_STR("bootimage",  "none",       CFGF_NONE),
	CFG_SEC("image",      image_opts,   CFGF_MULTI | CFGF_TITLE),
	CFG_SEC("network",    network_opts, CFGF_MULTI | CFGF_TITLE),
	CFG_STR("line",       "none",       CFGF_LIST),
	CFG_STR("password",   "none",       CFGF_NONE),
	CFG_END()
    };

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
	    MB_DEBUG("[mb] check_last: switching from default to fallback.\n");
	    cfg_setstr(cfg, "bootimage", cfg_getstr(cfg, "fallback"));
	}

	if (strcmp(cfg_getstr(cfg, "lasttry"), cfg_getstr(cfg, "fallback")) == 0) {
	    /* we just tried our fallback image and it failed. yikes.
	       don't do anything, sit here and wait for someone to
	       help us out. */
	    MB_DEBUG("[mb] check_last: fallback failed, panicking.\n");

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
	    MB_DEBUG("[mb] check_last: fallback succeeded, switching back to default\n");
	}

	return 1;
    }
}

/* write config back to file, with shimmying */
void save_config(cfg_t *cfg) {
    cmd_write(cfg, NULL);
}

/* clean up */
void drop_config(cfg_t *cfg) {
    cfg_free(cfg);
}

/* set/unset ICANON flag */
void set_canonical(int flag) {
    struct termios term;
    
    if (tcgetattr(STDIN_FILENO, &term) < 0) {
	fprintf(stderr, "unable to tcgetattr: %s\n", strerror(errno));
    }

    if (flag) {
	term.c_lflag |= ICANON;
    } else {
	term.c_lflag &= ~ICANON;
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
	fprintf(stderr, "unable to tcsetattr: %s\n", strerror(errno));
    }
}

/* get keypress with timeout */

static jmp_buf env_alrm;
static void sig_alrm(int signo) {
    longjmp(env_alrm, 1);
}
  
int get_keypress (int delay) {
    int c;
    
    if (!delay)
	return 1;

    set_canonical(0);
    if (setvbuf(stdin, NULL, _IONBF, 0) < 0) {
	fprintf(stderr, "unable to setvbuf: %s\n", strerror(errno));
    }
    
    printf("MONOBOOT booting in %ds\npress a key for a shell", delay);
    while (delay--) {
	printf(".");
        if (signal(SIGALRM, sig_alrm) == SIG_ERR) {
	    fprintf(stderr, "signal: %s\n", strerror(errno));
	    set_canonical(1);
	    return(-1);
	}
	if (setjmp(env_alrm) == 0) {
	    alarm(1);
	    c = getchar();
	}
	if (c) {
	    alarm(0);
	    printf("\ngot keypress\n");
	    set_canonical(1);
	    return(0);
	}
    }

    set_canonical(1);
    return(1);
}
