#define C99_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <confuse.h>
#include "monoboot.h"
#include "monoboot_cmds.h"
#include "monoboot_cli.h"

/* $Id$ */

/*
 * these two functions are ripped off from confuse.c ....
 * for some reason there's no public function to add a 
 * section to the in-memory config
 * ...
 */
  
static cfg_opt_t *cfg_dupopts(cfg_opt_t *opts)
{
	int n;
	cfg_opt_t *dupopts;

	for(n = 0; opts[n].name; n++)
		/* do nothing */ ;
	dupopts = (cfg_opt_t *)malloc(++n * sizeof(cfg_opt_t));
	memcpy(dupopts, opts, n * sizeof(cfg_opt_t));
	return dupopts;
}

void add_image(cfg_t *cfg, char *image) {
    cfg_opt_t *opt;
    cfg_value_t *val;
    int i;
    
    for (i = 0; cfg->opts[i].type != CFGT_NONE; i++) {
	if (strncmp(cfg->opts[i].name, "image", 5) == 0) {
	    opt = &cfg->opts[i];
	}
    }
    opt->values = (cfg_value_t **)realloc(opt->values, (opt->nvalues+1) * sizeof(cfg_value_t *));
    opt->values[opt->nvalues] = (cfg_value_t *)malloc(sizeof(cfg_value_t));
    memset(opt->values[opt->nvalues], 0, sizeof(cfg_value_t));
    val = opt->values[opt->nvalues++];
    
    cfg_free(val->section);
    val->section = (cfg_t *)malloc(sizeof(cfg_t));
    memset(val->section, 0, sizeof(cfg_t));
    val->section->opts = cfg_dupopts(opt->subopts);
    val->section->flags = cfg->flags;
    val->section->flags |= CFGF_ALLOCATED;
    val->section->filename = cfg->filename;
    val->section->line = cfg->line;
    val->section->errfunc = cfg->errfunc;
    val->section->name = strdup(opt->name);
    val->section->title = strndup(image, strlen(image));
}

int check_image_tag(cfg_t *cfg, char *tag) { 
    int n, m;
    int images;

    images = cfg_size(cfg, "image");
    m = 0;
    for (n = 0; n < images; n++) {
	cfg_t *image = cfg_getnsec(cfg, "image", n);
	if (strncmp(cfg_title(image), tag, strlen(cfg_title(image))) == 0) {
	    m++;
	}
    }
    if (m) {
	return 1;
    } else {
	return 0;
    }
}

char *get_running_config(cfg_t *cfg) {
    char config[MB_CONFIG_MAX];
    char buf[MB_CONFIG_MAX];
    int sec_count, n;

    sprintf(config, "version = %ld\n\nbootonce = %s\ntryonce = %s\n\nfallback = %s\ndefault = %s\n\nlasttry = %s\nlastboot = %s\n\nbootimage = %s\n", 
	    cfg_getint(cfg,"version"), 
	    cfg_getstr(cfg,"bootonce"),
	    cfg_getstr(cfg,"tryonce"),
	    cfg_getstr(cfg,"fallback"),
	    cfg_getstr(cfg,"default"),
	    cfg_getstr(cfg,"lasttry"),
	    cfg_getstr(cfg,"lastboot"),
	    cfg_getstr(cfg,"bootimage"));
    
    sec_count = cfg_size(cfg, "image");
    for (n = 0; n < sec_count; n++) {
	cfg_t *image = cfg_getnsec(cfg, "image", n);
	sprintf(buf, "\nimage %s {\n  filename = %s\n}\n", 
		cfg_title(image), 
		cfg_getstr(image, "filename"));
	strncat(config, buf, strlen(buf));
    }

    sec_count = cfg_size(cfg, "network");
    for (n = 0; n < sec_count; n++) {
	cfg_t *network = cfg_getnsec(cfg, "network", n);
	sprintf(buf, "\nnetwork %s {\n  address = %s\n  netmask = %s\n  gateway = %s\n}\n\n", 
		cfg_title(network), 
		cfg_getstr(network, "address"),
		cfg_getstr(network, "netmask"),
		cfg_getstr(network, "gateway"));
	strncat(config, buf, strlen(buf));
    }

    strcat(config, "line = { ");
    sec_count = cfg_size(cfg, "line");
    for (n = 0; n < sec_count; n++) {
	if (n == (sec_count - 1)) {
	    sprintf(buf, "%s", cfg_getnstr(cfg, "line", n));
	} else {
	    sprintf(buf, "%s, ", cfg_getnstr(cfg, "line", n));
	}
	strncat(config, buf, strlen(buf));
    }
    strcat(config, " }\n\n");
    
    sprintf(buf, "password = %s\n\n", cfg_getstr(cfg, "password"));
    strncat(config, buf, strlen(buf));
    
    return strdup(config);
}

void cmd_boot(cfg_t *cfg, char **cmdline) {
    
    pid_t pid, status;
    int n, images;
    char *image_tag;
    char image_file[256];
  
    /* if the user said "boot <something>" then check that tag exists,
       then boot it, else default tag */

    if (cmdline != NULL && cmdline[1] != NULL) {
	/* check */
	if (check_image_tag(cfg, cmdline[1])) {
	    image_tag = cmdline[1];
	} else {
	    printf("no such tag %s\n", cmdline[1]);
	    return;
	}
    } else {
	image_tag = cfg_getstr(cfg, "bootimage");
    }

    if (strncmp(cfg_getstr(cfg, "tryonce"), "yes", 3) == 0) {
	image_tag = cfg_getstr(cfg, "bootonce");
    }

    MB_DEBUG("[mb] image_tag: %s\n", image_tag);
    
    /* set lasttry to the image tag we're about to boot, and
       lastboot to nil, so we'll see if the boot failed */
    cfg_setstr(cfg, "lasttry", image_tag);
    cfg_setstr(cfg, "lastboot", "nil");
    save_config(cfg);

    /* must call cfg_size again, if we're going to use cfg_getnsec (it
       seems) */
    images = cfg_size(cfg, "image");
    for (n = 0; n < images; n++) {
	cfg_t *image = cfg_getnsec(cfg, "image", n);
	if (strncmp(cfg_title(image), image_tag, strlen(cfg_title(image))) == 0) {
	    strcpy(image_file, cfg_getstr(image, "filename"));
	}
    }

    MB_DEBUG("[mb] image_file: %s\n", image_file);

    /* fork/exec the kexec -l first */
    if ( (pid = fork()) < 0) {
	MB_DEBUG("[mb] boot_image: fork failed: %s\n", strerror(errno));
	return;
    } else if (pid == 0) {
	MB_DEBUG("[mb] boot_image: booting image file %s\n", image_file);
	if (execlp("KEXEC_BINARY", "kexec", "-l", image_file, (char *) 0) < 0) {
	    MB_DEBUG("[mb] boot_image: exec of %s failed: %s\n", KEXEC_BINARY, strerror(errno));
	    exit(1);
	}
    }
    if (waitpid(pid, &status, 0) < 0) {
	MB_DEBUG("[mb] boot_image: wait failed: %s\n", strerror(errno));
	return;
    } else if (WIFEXITED(status)) {
	MB_DEBUG("[mb] child exited ... \n");
	if (WEXITSTATUS(status)) {
	    MB_DEBUG("[mb] ... non-zero, bailing out\n");
	    return;
	} else {
	    MB_DEBUG("[mb] ... zero, looks good\n");
	}
    } else {
	MB_DEBUG("[mb] child died, bailing\n");
	return;
    }

    /* drop the confuse config stuff now, 'cos we're fairly sure this
       exec will work ok. */
    drop_config(cfg);

    /* now exec the kexec -e -- hardly worth forking */
    execlp("KEXEC_BINARY", "kexec", "-e", (char *) 0);

    /* can't happen; we're off booting the new kernel (right?) */
    MB_DEBUG("[mb] boot_image: we're still here. exec of %s failed: %s\n", KEXEC_BINARY, strerror(errno));
    return;
}


void cmd_show(cfg_t *cfg, char **cmdline) {
    char *config;
    /* we expect either 'show running-config' or 'show startup-config' */
    if (cmdline[1] == NULL) {
	return;
    } else {
	if (strncmp(cmdline[1], "r", 1) == 0) {
	    /* show run */
	    config = get_running_config(cfg);
	    printf("%s", config);
	}
	if (strncmp(cmdline[1], "s", 1) == 0) {
	    /* show start */

	}
    }
}

void cmd_copy(cfg_t *cfg, char **cmdline) {
}

void cmd_exit(cfg_t *cfg, char **cmdline) {
    if (get_mb_mode() == MB_MODE_CONF) {
	set_mb_mode(MB_MODE_EXEC);
	set_mb_prompt("mb> ");
    } else if (get_mb_mode() == MB_MODE_EXEC) {
	MB_DEBUG("would exit here, with cleanup\n");
    } else if (get_mb_mode() == MB_MODE_CONF_IMAGE) {
	set_mb_mode(MB_MODE_CONF);
	set_mb_prompt("conf> ");
    } else {
	/* not sure what mode we're in then */
    }
}

void cmd_conf(cfg_t *cfg, char **cmdline) {
    char prompt[MB_PROMPT_MAX];
    char image_name[MB_PROMPT_MAX];
    char *current_image;
    cfg_t *image;
    int images, n, m;

    if (get_mb_mode() == MB_MODE_EXEC) {
	/* set config mode */
	set_mb_prompt("conf> ");
	set_mb_mode(MB_MODE_CONF);
    } else if (get_mb_mode() == MB_MODE_CONF) {

	/* actually process config commands */
	if (cmdline[0]) {
	    
	    if (strncmp(cmdline[0], "bootonce", 8) == 0) {
		if (cmdline[1] && check_image_tag(cfg, cmdline[1])) {
		    cfg_setstr(cfg, "bootonce", cmdline[1]);
		    printf("%s -> %s [ok]\n", "bootonce", cmdline[1]);
		} else {
		    printf("no such tag %s\n", cmdline[1]);
		}
	    }

	    if (strncmp(cmdline[0], "default", 7) == 0) {
		if (cmdline[1] && check_image_tag(cfg, cmdline[1])) {
		    cfg_setstr(cfg, "default", cmdline[1]);
		    printf("%s -> %s [ok]\n", "default", cmdline[1]);
		} else {
		    printf("no such tag %s\n", cmdline[1]);
		}
		
	    }

	    if (strncmp(cmdline[0], "fallback", 8) == 0) {
		if (cmdline[1] && check_image_tag(cfg, cmdline[1])) {
		    cfg_setstr(cfg, "fallback", cmdline[1]);
		    printf("%s -> %s [ok]\n", "fallback", cmdline[1]);
		} else {
		    printf("no such tag %s\n", cmdline[1]);
		}
	    }

	    if (strncmp(cmdline[0], "tryonce", 7) == 0) {
		if (cmdline[1] && (strncmp(cmdline[1], "yes", 3) == 0 || strncmp(cmdline[1], "no", 2) == 0)) {
		    cfg_setstr(cfg, "tryonce", cmdline[1]);
		    printf("%s -> %s [ok]\n", "tryonce", cmdline[1]);
		} else {
		    printf("tryonce must be 'yes' or 'no'\n");
		}
	    }

	    if (strncmp(cmdline[0], "bootimage", 9) == 0) {
		if (cmdline[1] && check_image_tag(cfg, cmdline[1])) {
		    cfg_setstr(cfg, "bootimage", cmdline[1]);
		    printf("%s -> %s [ok]\n", "bootimage", cmdline[1]);
		} else {
		    printf("no such tag %s\n", cmdline[1]);
		}
	    }

	    if (strncmp(cmdline[0], "image", 5) == 0) {
		if (cmdline[1]) {
		    /* set image config mode */

		    /* truncate image name */
		    strncpy(image_name, cmdline[1], MB_PROMPT_MAX - 7);
		    if (strlen(cmdline[1]) < (MB_PROMPT_MAX - 7)) {
			image_name[strlen(cmdline[1])] = '\0';
		    } else {
			image_name[(MB_PROMPT_MAX - 7)] = '\0';
		    }
		    /* set the prompt to include the image name */
		    sprintf(prompt, "conf-%s> ", image_name);
		    set_mb_prompt(prompt);
		    set_mb_mode(MB_MODE_CONF_IMAGE);
		    set_mb_image(image_name);
		}
	    }
	}
    } else if (get_mb_mode() == MB_MODE_CONF_IMAGE) {
        current_image = get_mb_image();
	if (strncmp(cmdline[0], "filename", 8) == 0) {
	    if (cmdline[1]) {
		MB_DEBUG("updating filename to %s for image %s\n", cmdline[1], current_image);
		images = cfg_size(cfg, "image");
		m = 0;
		for (n = 0; n < images; n++) {
		    image = cfg_getnsec(cfg, "image", n);
		    if (strncmp(cfg_title(image), current_image, strlen(cfg_title(image))) == 0) {
			cfg_setstr(image, "filename", cmdline[1]);
			m++;
		    }
		}
		if (m) {
		    printf("%s/filename -> %s [ok]\n", current_image, cmdline[1]);
		} else {
		    /* new image section */
		    cfg_opt_t *new_image = malloc(sizeof(cfg_opt_t));
		    if (new_image == NULL) {
			MB_DEBUG("malloc failed: %s\n", strerror(errno));
			return;
		    }
		    
		    add_image(cfg, current_image);
		    printf("new image %s added [ok]\n", current_image);
		    
		    images = cfg_size(cfg, "image");
		    m = 0;
		    for (n = 0; n < images; n++) {
			image = cfg_getnsec(cfg, "image", n);
			if (strncmp(cfg_title(image), current_image, strlen(cfg_title(image))) == 0) {
			    cfg_setstr(image, "filename", cmdline[1]);
			    m++;
			}
		    }
		    if (m) {
			printf("%s/filename -> %s [ok]\n", current_image, cmdline[1]);
		    } else {
			MB_DEBUG("set filename failed\n");
		    }
		}
	    }
	}
    }    
}

void cmd_write(cfg_t *cfg, char **cmdline) {
    FILE *fp = fopen(MB_CONF_NEW, "w");
    char *config;
    char timestr[MB_TIMESTR_MAX];
    time_t t;

    MB_DEBUG("[mb] cmd_write: writing config to %s\n", MB_CONF_NEW);
    if (!fp) {
	MB_DEBUG("[mb] cmd_write: open failed: %s\n", strerror(errno));
	return;
    }
    
    t = time(NULL);
    if (!strftime(timestr, MB_TIMESTR_MAX, "%a, %d %b %Y %H:%M:%S %z", 
		  localtime(&t))) {
	MB_DEBUG("[mb] cmd_write: strftime failed.\n");
    }

    config = get_running_config(cfg);
    fprintf(fp, "# config saved by monoboot at %s\n\n", timestr);
    fputs(config, fp);
    fclose(fp);
    
    MB_DEBUG("[mb] cmd_write: preserving old config as %s\n", MB_CONF_OLD);
    if (rename(MB_CONF, MB_CONF_OLD)) {
	MB_DEBUG("[mb] cmd_write: rename failed: %s\n", strerror(errno));
    }
    MB_DEBUG("[mb] cmd_write: putting new config in place\n");
    if (rename(MB_CONF_NEW, MB_CONF)) {
	MB_DEBUG("[mb] cmd_write: rename failed: %s\n", strerror(errno));
    }
}
