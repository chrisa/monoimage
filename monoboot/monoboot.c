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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <confuse.h>
#include "monoboot.h"

int main(int argc, char **argv) {
    cfg_t *cfg;

    cfg = load_config(MB_CONF);
    show_config(cfg);
    if (check_last(cfg)) {
	MB_DEBUG("[mb] main: last boot ok, proceeding\n");
    } else {
	MB_DEBUG("[mb] main: last boot failed, changing to fallback\n");
	update_config_for_fallback(cfg);
    }
    boot_image(cfg);
    return 0; /* probably not */
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
    MB_DEBUG("[mb] boot_image: booting from default image %s\n", cfg_getstr(cfg, "default"));
    cfg_setstr(cfg, "last", cfg_getstr(cfg, "default"));

    save_config(cfg);
    drop_config(cfg);
    
    /* do it */
    execlp(KEXEC_BINARY, "-l", cfg_getstr(cfg, "default"));
    execlp(KEXEC_BINARY, "-e");

    /* can't happen; we're off booting the new kernel (right?) */
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
