#define C99_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <confuse.h>
#include "monoboot.h"

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

void add_section(cfg_t *cfg, char *name, char *title) {
    cfg_opt_t *opt;
    cfg_value_t *val;
    int i;
    
    for (i = 0; cfg->opts[i].type != CFGT_NONE; i++) {
	if (strncmp(cfg->opts[i].name, name, strlen(name)) == 0) {
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
    val->section->filename = cfg->filename ? strdup(cfg->filename) : 0;
    val->section->line = cfg->line;
    val->section->errfunc = cfg->errfunc;
    val->section->name = strdup(opt->name);
    val->section->title = strndup(title, strlen(title));
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

char *get_startup_config(cfg_t *cfg) {
    char config[MB_CONFIG_MAX];
    char buf[MB_CONFIG_MAX];
    FILE *fp = fopen(MB_CONF, "r");
    while(fgets(buf, MB_CONFIG_MAX, fp) != NULL) {
	strncat(config, buf, strlen(buf));
    }
    fclose(fp);
    return strdup(config);
}

char *get_running_config(cfg_t *cfg) {
    char config[MB_CONFIG_MAX];
    char buf[MB_CONFIG_MAX];
    int sec_count, n;

    sprintf(config, "version = %ld\ndelay = %ld\n\nbootonce = %s\ntryonce = %s\n\nfallback = %s\ndefault = %s\n\nlasttry = %s\nlastboot = %s\n\nbootimage = %s\n", 
	    cfg_getint(cfg,"version"), 
	    cfg_getint(cfg,"delay"), 
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
	sprintf(buf, "\nnetwork %s {\n  address = %s\n  gateway = %s\n}\n\n", 
		cfg_title(network), 
		cfg_getstr(network, "address"),
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

int cmd_boot(cfg_t *cfg, char **cmdline) {
    int n, images;
    char *image_tag;
    char image_file[MB_PATH_MAX];
    char image_path[MB_PATH_MAX];
    struct stat sbuf;
  
    /* if the user said "boot <something>" then check that tag exists,
       then boot it, else default tag */

    if (cmdline != NULL && cmdline[1] != NULL) {
	/* check */
	if (check_image_tag(cfg, cmdline[1])) {
	    image_tag = cmdline[1];
	} else {
	    printf("no such tag %s\n", cmdline[1]);
	    return -1;
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

    /* check the filename we're being asked to boot is there. */
    sprintf(image_path, "%s/%s", MB_PATH_IMAGES, image_file);
    if (stat(image_path, &sbuf) < 0) {
	MB_DEBUG("[mb] cmd_boot: couldn't stat %s: %s", image_path, strerror(errno));
	return -1;
    }
    MB_DEBUG("[mb] image path: %s\n", image_path);

    /* get /proc mounted for kexec's benefit */
    if (do_exec(MOUNT_BINARY, "mount", "-t", "proc", "proc", "/proc", 0) != 0) {
	MB_DEBUG("[mb] mount /proc failed - already mounted?\n");
    }

    /* fork/exec the kexec -l first */
    if (do_exec(KEXEC_BINARY, "kexec", "-l", image_path, 0) != 0) {
	return -1;
    }

    /* drop the confuse config stuff now, 'cos we're fairly sure this
       exec will work ok. */
    drop_config(cfg);

    /* now exec the kexec -e -- hardly worth forking */
    execlp(KEXEC_BINARY, "kexec", "-e", (char *) 0);

    /* can't happen; we're off booting the new kernel (right?) */
    MB_DEBUG("[mb] boot_image: we're still here. exec of %s failed: %s\n", KEXEC_BINARY, strerror(errno));
    return -1;
}

int cmd_show(cfg_t *cfg, char **cmdline) {
    char *p;

    if (cmdline[1] == NULL) {
	return -1;
    } else {
	if (strncmp(cmdline[1], "r", 1) == 0) {
	    /* show run */
	    p = get_running_config(cfg);
	}
	if (strncmp(cmdline[1], "s", 1) == 0) {
	    /* show start */
	    p = get_startup_config(cfg);
	}
	if (strncmp(cmdline[1], "v", 1) == 0) {
	    p = strdup("monoboot v0\n");
	}
	printf("%s", p);
    }
    return 0;
}

int cmd_copy(cfg_t *cfg, char **cmdline) {
    if (cmdline[1] && cmdline[2]) {
	if (strncmp(cmdline[1], "tftp", 4) == 0 || 
	    strncmp(cmdline[2], "tftp", 4) == 0 ) {
	    if (do_tftp(cfg, cmdline[1], cmdline[2]) == 0) {
		printf("[ok]\n");
	    } else {
		printf("[fail]\n");
	    }
	}
    }
    return 0;
}

int cmd_exit(cfg_t *cfg, char **cmdline) {
    if (get_mb_mode() == MB_MODE_CONF) {
	set_mb_mode(MB_MODE_EXEC);
	set_mb_prompt("mb> ");
    } else if (get_mb_mode() == MB_MODE_EXEC) {
	MB_DEBUG("would exit here, with cleanup\n");
    } else if (get_mb_mode() == MB_MODE_CONF_IMAGE) {
	set_mb_mode(MB_MODE_CONF);
	set_mb_prompt("conf> ");
    } else if (get_mb_mode() == MB_MODE_CONF_NET) {
	set_mb_mode(MB_MODE_CONF);
	set_mb_prompt("conf> ");
	do_netconf(cfg);
    } else {
	/* not sure what mode we're in then */
    }
    return 0;
}

int cmd_conf(cfg_t *cfg, char **cmdline) {
    char prompt[MB_PROMPT_MAX];
    char section_name[MB_PROMPT_MAX];
    char *current_section;
    cfg_t *section;
    int sections, n, m;

    if (get_mb_mode() == MB_MODE_EXEC) {
	/* set config mode */
	set_mb_prompt("conf> ");
	set_mb_mode(MB_MODE_CONF);
    } else if (get_mb_mode() == MB_MODE_CONF) {

	/* actually process config commands */
	if (cmdline[0]) {
	    
	    if (strncmp(cmdline[0], "delay", 5) == 0) {
		if (cmdline[1] && atoi(cmdline[1]) >= 0) {
		    cfg_setint(cfg, "delay", atoi(cmdline[1]));
		    printf("%s -> %ld [ok]\n", "delay", cfg_getint(cfg, "delay"));
		}
	    }

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

	    if (strncmp(cmdline[0], "tryonce", 7) == 0) {
		if (cmdline[1] && (strncmp(cmdline[1], "yes", 3) == 0 || strncmp(cmdline[1], "no", 2) == 0)) {
		    cfg_setstr(cfg, "tryonce", cmdline[1]);
		    printf("%s -> %s [ok]\n", "tryonce", cmdline[1]);
		} else {
		    printf("tryonce must be 'yes' or 'no'\n");
		}
	    }

	    if (strncmp(cmdline[0], "password", 8) == 0) {
		if (cmdline[1]) {
		    cfg_setstr(cfg, "password", cmdline[1]);
		    printf("%s -> %s [ok]\n", "password", cmdline[1]);
		}
	    }

	    if (strncmp(cmdline[0], "image", 5) == 0) {
		if (cmdline[1]) {
		    /* set image config mode */

		    /* truncate image name */
		    strncpy(section_name, cmdline[1], MB_PROMPT_MAX - 7);
		    if (strlen(cmdline[1]) < (MB_PROMPT_MAX - 7)) {
			section_name[strlen(cmdline[1])] = '\0';
		    } else {
			section_name[(MB_PROMPT_MAX - 7)] = '\0';
		    }
		    /* set the prompt to include the image name */
		    sprintf(prompt, "conf-%s> ", section_name);
		    set_mb_prompt(prompt);
		    set_mb_mode(MB_MODE_CONF_IMAGE);
		    set_mb_image(section_name);
		}
	    }

	    if (strncmp(cmdline[0], "network", 7) == 0) {
		if (cmdline[1]) {
		    /* set network config mode */

		    /* truncate network name */
		    strncpy(section_name, cmdline[1], MB_PROMPT_MAX - 7);
		    if (strlen(cmdline[1]) < (MB_PROMPT_MAX - 7)) {
			section_name[strlen(cmdline[1])] = '\0';
		    } else {
			section_name[(MB_PROMPT_MAX - 7)] = '\0';
		    }
		    /* set the prompt to include the network name */
		    sprintf(prompt, "conf-%s> ", section_name);
		    set_mb_prompt(prompt);
		    set_mb_mode(MB_MODE_CONF_NET);
		    set_mb_network(section_name);
		}
	    }
	}
    } else if (get_mb_mode() == MB_MODE_CONF_IMAGE) {
        current_section = get_mb_image();
	if (strncmp(cmdline[0], "filename", 8) == 0) {
	    if (cmdline[1]) {
		MB_DEBUG("updating filename to %s for image %s\n", cmdline[1], current_section);
		sections = cfg_size(cfg, "image");
		m = 0;
		for (n = 0; n < sections; n++) {
		    section = cfg_getnsec(cfg, "image", n);
		    if (strncmp(cfg_title(section), current_section, strlen(cfg_title(section))) == 0) {
			cfg_setstr(section, "filename", cmdline[1]);
			m++;
		    }
		}
		if (m) {
		    printf("%s/filename -> %s [ok]\n", current_section, cmdline[1]);
		} else {
		    /* new image section */
		    add_section(cfg, "image", current_section);
		    printf("new image %s added [ok]\n", current_section);
		    
		    sections = cfg_size(cfg, "image");
		    m = 0;
		    for (n = 0; n < sections; n++) {
			section = cfg_getnsec(cfg, "image", n);
			if (strncmp(cfg_title(section), current_section, strlen(cfg_title(section))) == 0) {
			    cfg_setstr(section, "filename", cmdline[1]);
			    m++;
			}
		    }
		    if (m) {
			printf("%s/filename -> %s [ok]\n", current_section, cmdline[1]);
		    } else {
			MB_DEBUG("set filename failed\n");
		    }
		}
	    }
	}
    } else if (get_mb_mode() == MB_MODE_CONF_NET) {
        current_section = get_mb_network();
	if (strncmp(cmdline[0], "address", 7) == 0 || 
	    strncmp(cmdline[0], "gateway", 7) == 0 ) {
	    if (cmdline[1]) {
		
		sections = cfg_size(cfg, "network");
		m = 0;
		for (n = 0; n < sections; n++) {
		    section = cfg_getnsec(cfg, "network", n);
		    if (strncmp(cfg_title(section), current_section, strlen(cfg_title(section))) == 0) {
			cfg_setstr(section, cmdline[0], cmdline[1]);
			m++;
		    }
		}
		if (m) {
		    printf("%s/%s -> %s [ok]\n", current_section, cmdline[0], cmdline[1]);
		} else {
		    /* new network section */
		    add_section(cfg, "network", current_section);
		    printf("new network %s added [ok]\n", current_section);
		    
		    sections = cfg_size(cfg, "network");
		    m = 0;
		    for (n = 0; n < sections; n++) {
			section = cfg_getnsec(cfg, "network", n);
			if (strncmp(cfg_title(section), current_section, strlen(cfg_title(section))) == 0) {
			    cfg_setstr(section, cmdline[0], cmdline[1]);
			    m++;
			}
		    }
		    if (m) {
			printf("%s/%s -> %s [ok]\n", current_section, cmdline[0], cmdline[1]);
		    } else {
			MB_DEBUG("set %s failed\n", cmdline[0]);
		    }
		}
	    }
	}
    }    
    return 0;
}

int cmd_write(cfg_t *cfg, char **cmdline) {
    FILE *fp;
    char *config;
    char timestr[MB_TIMESTR_MAX];
    char conf_new_path[MB_PATH_MAX];
    char conf_old_path[MB_PATH_MAX];
    char conf_path[MB_PATH_MAX];
    time_t t;

    sprintf(conf_new_path, "%s/%s", MB_PATH_CONFIG, MB_CONF_NEW);
    sprintf(conf_old_path, "%s/%s", MB_PATH_CONFIG, MB_CONF_OLD);
    sprintf(conf_path, "%s/%s", MB_PATH_CONFIG, MB_CONF);

    MB_DEBUG("[mb] cmd_write: writing config to %s\n", conf_new_path);
    fp = fopen(conf_new_path, "w");
    if (!fp) {
	MB_DEBUG("[mb] cmd_write: open %s for write failed: %s\n", conf_new_path, strerror(errno));
	return -1;
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
    
    MB_DEBUG("[mb] cmd_write: preserving old config as %s\n", conf_old_path);
    if (rename(conf_path, conf_old_path)) {
	MB_DEBUG("[mb] cmd_write: rename failed: %s\n", strerror(errno));
    }
    MB_DEBUG("[mb] cmd_write: putting new config in place\n");
    if (rename(conf_new_path, conf_path)) {
	MB_DEBUG("[mb] cmd_write: rename failed: %s\n", strerror(errno));
    }
    return 0;
}
