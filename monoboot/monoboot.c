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

/*
 * XXX: exit(3) is nfg in this scenario
 *      path the config file into the rw filesys
 *	logic for third image
 *	datestamp the config upon write
 */

#define C99_SOURCE

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <confuse.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <malloc.h>
#include "monoboot.h"

/* global pointer for lines read with readline */
char *line_read = (char *)NULL;

int main(int argc, char **argv) {
    cfg_t *cfg;
    int interact = 1;

    if (!strncmp(argv[0],"mbsh",4)) {
	interact = 1;
    } else if (!strncmp(argv[0],"monoboot",8)) {
	interact = 0;
    } else {
	/* unsure what we've been run as */
	interact = 1;
    }

    cfg = load_config(MB_CONF);
    show_config(cfg);
    if (check_last(cfg)) {
	MB_DEBUG("[mb] main: last boot ok, proceeding\n");
    } else {
	MB_DEBUG("[mb] main: last boot failed, changing to fallback\n");
	update_config_for_fallback(cfg);
    }

/*     opterr = 0; */
/*     while ( (c = getopt(argc, argv, "b") ) != EOF ) { */
/* 	switch (c) { */
/* 	case 'b': /\* boot default straightaway *\/ */
/* 	    break; */
/* 	default: */
/* 	    break; */
/* 	} */
/*     } */

    if (interact == 1) {
	MB_DEBUG("[mb] main: starting interactive cmdline\n");
	mb_interact(cfg);
    } else {
	MB_DEBUG("[mb] main: booting image\n");
	boot_image(cfg);
    }
    return 0;
}

/* 
 * Read a string, and return a pointer to it.
 * Returns NULL on EOF. 
 */

char *rl_gets () {
    /* If the buffer has already been allocated,
       return the memory to the free pool. */
    if (line_read) {
	free (line_read);
	line_read = (char *)NULL;
    }
    
    /* Get a line from the user. */
    line_read = readline ("mb> ");
    
    /* If the line has any text in it,
       save it on the history. */
    if (line_read && *line_read)
	add_history (line_read);
    
    return (line_read);
}

/* 
 * set up libreadline - 
 *  turn off tabbing of filenames
 *  custom complete?
 */

void init_readline (void) {
    rl_bind_key ('\t', rl_insert);
}

/* 
 * loop reading commands from the user, until 
 * they either reboot or start a kernel 
 */
void mb_interact(cfg_t *cfg) {
    init_readline();

    while ( (line_read = rl_gets()) != NULL) {
	if ( strncmp(line_read, "boot", 4) == 0 ) {
	    boot_image(cfg);
	}
    }
    MB_DEBUG("\n[mb] mb_interact: exiting mb_interact\n");
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
	CFG_INT("version", 0, CFGF_NONE),
	CFG_STR("default", "none", CFGF_NONE),
	CFG_STR("fallback", "none", CFGF_NONE),
	CFG_STR("last", "none", CFGF_NONE),
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

/* just list the configs */
void show_config(cfg_t *cfg) {
    int images, n;

    images = cfg_size(cfg, "image");
    MB_DEBUG("[mb] show_config: total %d image(s)\n", images);

    for (n = 0; n < images; n++) {
	cfg_t *image = cfg_getnsec(cfg, "image", n);
	MB_DEBUG("[mb] show_config: image %s has filename %s\n", cfg_title(image), cfg_getstr(image, "filename"));
    }

    MB_DEBUG("[mb] show_config: last image is %s\n", cfg_getstr(cfg, "last"));
    MB_DEBUG("[mb] show_config: default image is %s\n", cfg_getstr(cfg, "default"));
    MB_DEBUG("[mb] show_config: fallback image is %s\n", cfg_getstr(cfg, "fallback"));

    return;
}

/* examine last boot's success */
int check_last(cfg_t *cfg) {
    if (!strcmp(cfg_getstr(cfg, "last"), cfg_getstr(cfg, "default"))) {
    	MB_DEBUG("[mb] check_last: last boot was from default image, good\n");

	/* check that it actually booted ok */
	/* if it didn't, return FALSE to indicate we should change to fallback */

	return 1;
    } else if (!strcmp(cfg_getstr(cfg, "last"), cfg_getstr(cfg, "fallback"))) {
    	MB_DEBUG("[mb] check_last: last boot was from fallback image, hmm\n");

	/* having booted from fallback, we're up for trying default again */

	return 1;
    } else {
        MB_DEBUG("[mb] check_last: last boot was from neither default nor fallback, kinky\n");

	/* check that it actually booted ok */
	/* if it didn't, return FALSE to indicate we should change to fallback */
	/* if it did, we're going to boot from the default anyway */
	/* XXX maybe not */

	return 1;
    }

    return 0;
}

/* change our config so we boot off the fallback image */
void update_config_for_fallback(cfg_t *cfg) {
    MB_DEBUG("[mb] update_config_for_fallback: changing default boot image to fallback image %s\n", cfg_getstr(cfg, "fallback"));
    cfg_setstr(cfg, "default", cfg_getstr(cfg, "fallback"));
    return;
}

/* housekeep, do the kexec */
void boot_image(cfg_t *cfg) {
    
    pid_t pid;
    int n,images;
    static char image_file[256];
  
    cfg_setstr(cfg, "last", cfg_getstr(cfg, "default"));

    images = cfg_size(cfg, "image");
    for (n = 0; n < images; n++) {
	cfg_t *image = cfg_getnsec(cfg, "image", n);
	if (strncmp(cfg_title(image), cfg_getstr(cfg, "default"), strlen(cfg_title(image))) == 0) {
	    strcpy(image_file, cfg_getstr(image, "filename"));
	}
    }

    save_config(cfg);
    drop_config(cfg);

    /* fork/exec the kexec -l first */
    if ( (pid = fork()) < 0) {
	MB_DEBUG("[mb] boot_image: fork failed: %s\n", strerror(errno));
	exit (1);
    } else if (pid == 0) {
	MB_DEBUG("[mb] boot_image: booting from default image file %s\n", image_file);
	if (execlp("KEXEC_BINARY", "kexec", "-l", image_file, (char *) 0) < 0) {
	    MB_DEBUG("[mb] boot_image: exec failed: %s\n", strerror(errno));
	    exit (1);
	}
    }
    if (waitpid(pid, NULL, 0) < 0) {
	MB_DEBUG("[mb] boot_image: wait failed: %s\n", strerror(errno));
	exit (1);
    }
    
    /* now exec the kexec -e -- hardly worth forking */
    execlp("KEXEC_BINARY", "kexec", "-e", (char *) 0);

    /* can't happen; we're off booting the new kernel (right?) */
    MB_DEBUG("[mb] boot_image: we're still here. kexec failed: %s\n", strerror(errno));
    return;
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
