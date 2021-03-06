#define C99_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <confuse.h>
#include <libcli.h>
#include "monoboot.h"

/* $Id$ */

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

int cmd_show_disk (struct cli_def *cli, cfg_t *cfg, char **cmdline) {
    struct stat s;
    struct statvfs sf;
    struct dirent *f;
    DIR *d;
    int i, used;

    d = opendir("/images");
    if (d == NULL) {
	MB_DEBUG("[mb] cmd_show_disk: failed to opendir /images: %s\n", strerror(errno));
        return -1;
    }
    chdir("/images");
    
    i = 1;
    used = 0;
    while ((f = readdir(d))) {
	if (strncmp(".", f->d_name, 1) != 0 && 
	    strncmp("..", f->d_name, 2) != 0 &&
	    strncmp("lost+found", f->d_name, 10) != 0) {
	    i++;
	    if (stat(f->d_name, &s) != 0) {
		MB_DEBUG("[mb] cmd_show_disk: stat failed: %s\n", strerror(errno));
		return -1;
	    }
	    fprintf(cli->client, "%10d %s\n", (int)s.st_size, f->d_name);
	    used += (int)s.st_size;
	}
    }
    chdir("/");

    if (statvfs("/images/.", &sf) != 0) {
	MB_DEBUG("[mb] cmd_show_disk: statfs failed: %s\n", strerror(errno));
	return -1;
    }
    fprintf(cli->client, "[%d bytes used, %ld available, %ld total]\n\n",
	    ( used ),
	    ( sf.f_bavail ),
	    ( sf.f_blocks ));
    
    return 0;
}

int cmd_show_start(struct cli_def *cli, cfg_t *cfg, char **cmdline) {
    FILE *fp = fopen(MB_CONF, "r");
    char buf[MB_CONFIG_MAX];
    while(fgets(buf, MB_CONFIG_MAX, fp) != NULL) {
            fprintf(cli->client, buf);
    }
    fclose(fp);
    return 0;
}

int cmd_show_run(struct cli_def *cli, cfg_t *cfg, char **cmdline) {
    int sec_count, n;

    fprintf(cli->client, "version = %ld\ndelay = %ld\n\nbootonce = %s\ntryonce = %s\n\nfallback = %s\ndefault = %s\n\nlasttry = %s\nlastboot = %s\n", 
	    cfg_getint(cfg,"version"), 
	    cfg_getint(cfg,"delay"), 
	    cfg_getstr(cfg,"bootonce"),
	    cfg_getstr(cfg,"tryonce"),
	    cfg_getstr(cfg,"fallback"),
	    cfg_getstr(cfg,"default"),
	    cfg_getstr(cfg,"lasttry"),
	    cfg_getstr(cfg,"lastboot"));
    
    sec_count = cfg_size(cfg, "image");
    for (n = 0; n < sec_count; n++) {
	cfg_t *image = cfg_getnsec(cfg, "image", n);
	fprintf(cli->client, "\nimage %s {\n  filename = %s\n}\n", 
		cfg_title(image), 
		cfg_getstr(image, "filename"));
    }

    sec_count = cfg_size(cfg, "network");
    for (n = 0; n < sec_count; n++) {
	cfg_t *network = cfg_getnsec(cfg, "network", n);
	fprintf(cli->client, "\nnetwork %s {\n  address = %s\n  gateway = %s\n}\n\n", 
		cfg_title(network), 
		cfg_getstr(network, "address"),
		cfg_getstr(network, "gateway"));
    }

    sec_count = cfg_size(cfg, "line");
    for (n = 0; n < sec_count; n++) {
	if (n == (sec_count - 1)) {
	    fprintf(cli->client, "%s", cfg_getnstr(cfg, "line", n));
	} else {
	    fprintf(cli->client, "%s, ", cfg_getnstr(cfg, "line", n));
	}
    }
    
    fprintf(cli->client, "password = %s\n\n", cfg_getstr(cfg, "password"));
    
    return 0;
}

int cmd_boot(struct cli_def *cli, cfg_t *cfg, char **cmdline) {
    int n, images;
    char *image_tag;
    char image_file[MB_PATH_MAX];
    char image_path[MB_PATH_MAX];
    char config_path[MB_PATH_MAX];
    struct stat sbuf;
    char *console;
    char kexec_cmdline[MB_CMDLINE_MAX];
  
    /* if the user said "boot <something>" then check that tag exists,
       then boot it, else default tag */

    if (cmdline != NULL && cmdline[1] != NULL) {
	MB_DEBUG("[mb] boot: tag %s\n", cmdline[1]);
	/* check */
	if (check_image_tag(cfg, cmdline[1])) {
	    image_tag = cmdline[1];
	} else {
	    printf("no such tag %s\n", cmdline[1]);
	    return -1;
	}
    } else {
	/* if the user didn't give us a tag name to boot
	   then use 'default' */
	image_tag = cfg_getstr(cfg, "default");
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

    /* check the config tag is there */
    sprintf(config_path, "%s/tag/%s", MB_PATH_CONFIG, image_tag);
    if (stat(config_path, &sbuf) < 0) {
	MB_DEBUG("[mb] cmd_boot: couldn't stat %s: %s", config_path, strerror(errno));
	return -1;
    }
    MB_DEBUG("[mb] config path: %s\n", config_path);

    /* get /proc mounted for kexec's benefit */
    if (check_mounted("/proc") != MB_CM_YES) {
	if (do_exec(MOUNT_BINARY, "mount", "-t", "proc", "proc", "/proc", 0) != 0) {
	    MB_DEBUG("[mb] mount /proc failed\n");
	}
    }

    /* find out what console we're currently using, assume that it'll
       do for the new kernel too */
    console = get_kernel_console();
    sprintf(kexec_cmdline, "--command-line=%s ide=nodma CONFIG=%s", console, config_path);
    
    /* fork/exec the kexec -l first */
    if (do_exec(KEXEC_BINARY, "kexec", kexec_cmdline, "-l", image_path, 0) != 0) {
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

int cmd_show_ver(struct cli_def *cli, cfg_t *cfg, char **cmdline)
{
        fprintf(cli->client, "monoboot v0\n");
        return 0;
}

int cmd_copy(struct cli_def *cli, cfg_t *cfg, char **cmdline) {
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

int cmd_conf(struct cli_def *cli, cfg_t *cfg, char **cmdline) {
    char prompt[MB_PROMPT_MAX];
    char section_name[MB_PROMPT_MAX];
    char *current_section;
    cfg_t *section;
    int sections, n, m;

/*     if (get_mb_mode() == MB_MODE_EXEC) { */
/* 	/\* set config mode *\/ */
/* 	set_mb_prompt("conf> "); */
/* 	set_mb_mode(MB_MODE_CONF); */
/*     } else if (get_mb_mode() == MB_MODE_CONF) { */

/* 	/\* actually process config commands *\/ */
/* 	if (cmdline[0]) { */
	    
/* 	    if (strncmp(cmdline[0], "delay", 5) == 0) { */
/* 		if (cmdline[1] && atoi(cmdline[1]) >= 0) { */
/* 		    cfg_setint(cfg, "delay", atoi(cmdline[1])); */
/* 		    printf("%s -> %ld [ok]\n", "delay", cfg_getint(cfg, "delay")); */
/* 		} */
/* 	    } */

/* 	    if (strncmp(cmdline[0], "bootonce", 8) == 0) { */
/* 		if (cmdline[1] && check_image_tag(cfg, cmdline[1])) { */
/* 		    cfg_setstr(cfg, "bootonce", cmdline[1]); */
/* 		    printf("%s -> %s [ok]\n", "bootonce", cmdline[1]); */
/* 		} else { */
/* 		    printf("no such tag %s\n", cmdline[1]); */
/* 		} */
/* 	    } */

/* 	    if (strncmp(cmdline[0], "default", 7) == 0) { */
/* 		if (cmdline[1] && check_image_tag(cfg, cmdline[1])) { */
/* 		    cfg_setstr(cfg, "default", cmdline[1]); */
/* 		    printf("%s -> %s [ok]\n", "default", cmdline[1]); */
/* 		} else { */
/* 		    printf("no such tag %s\n", cmdline[1]); */
/* 		} */
		
/* 	    } */

/* 	    if (strncmp(cmdline[0], "fallback", 8) == 0) { */
/* 		if (cmdline[1] && check_image_tag(cfg, cmdline[1])) { */
/* 		    cfg_setstr(cfg, "fallback", cmdline[1]); */
/* 		    printf("%s -> %s [ok]\n", "fallback", cmdline[1]); */
/* 		} else { */
/* 		    printf("no such tag %s\n", cmdline[1]); */
/* 		} */
/* 	    } */

/* 	    if (strncmp(cmdline[0], "tryonce", 7) == 0) { */
/* 		if (cmdline[1] && (strncmp(cmdline[1], "yes", 3) == 0 || strncmp(cmdline[1], "no", 2) == 0)) { */
/* 		    cfg_setstr(cfg, "tryonce", cmdline[1]); */
/* 		    printf("%s -> %s [ok]\n", "tryonce", cmdline[1]); */
/* 		} else { */
/* 		    printf("tryonce must be 'yes' or 'no'\n"); */
/* 		} */
/* 	    } */

/* 	    if (strncmp(cmdline[0], "tryonce", 7) == 0) { */
/* 		if (cmdline[1] && (strncmp(cmdline[1], "yes", 3) == 0 || strncmp(cmdline[1], "no", 2) == 0)) { */
/* 		    cfg_setstr(cfg, "tryonce", cmdline[1]); */
/* 		    printf("%s -> %s [ok]\n", "tryonce", cmdline[1]); */
/* 		} else { */
/* 		    printf("tryonce must be 'yes' or 'no'\n"); */
/* 		} */
/* 	    } */

/* 	    if (strncmp(cmdline[0], "password", 8) == 0) { */
/* 		if (cmdline[1]) { */
/* 		    cfg_setstr(cfg, "password", cmdline[1]); */
/* 		    printf("%s -> %s [ok]\n", "password", cmdline[1]); */
/* 		} */
/* 	    } */

/* 	    if (strncmp(cmdline[0], "image", 5) == 0) { */
/* 		if (cmdline[1]) { */
/* 		    /\* set image config mode *\/ */

/* 		    /\* truncate image name *\/ */
/* 		    strncpy(section_name, cmdline[1], MB_PROMPT_MAX - 7); */
/* 		    if (strlen(cmdline[1]) < (MB_PROMPT_MAX - 7)) { */
/* 			section_name[strlen(cmdline[1])] = '\0'; */
/* 		    } else { */
/* 			section_name[(MB_PROMPT_MAX - 7)] = '\0'; */
/* 		    } */
/* 		    /\* set the prompt to include the image name *\/ */
/* 		    sprintf(prompt, "conf-%s> ", section_name); */
/* 		    set_mb_prompt(prompt); */
/* 		    set_mb_mode(MB_MODE_CONF_IMAGE); */
/* 		    set_mb_image(section_name); */
/* 		} */
/* 	    } */

/* 	    if (strncmp(cmdline[0], "network", 7) == 0) { */
/* 		if (cmdline[1]) { */
/* 		    /\* set network config mode *\/ */

/* 		    /\* truncate network name *\/ */
/* 		    strncpy(section_name, cmdline[1], MB_PROMPT_MAX - 7); */
/* 		    if (strlen(cmdline[1]) < (MB_PROMPT_MAX - 7)) { */
/* 			section_name[strlen(cmdline[1])] = '\0'; */
/* 		    } else { */
/* 			section_name[(MB_PROMPT_MAX - 7)] = '\0'; */
/* 		    } */
/* 		    /\* set the prompt to include the network name *\/ */
/* 		    sprintf(prompt, "conf-%s> ", section_name); */
/* 		    set_mb_prompt(prompt); */
/* 		    set_mb_mode(MB_MODE_CONF_NET); */
/* 		    set_mb_network(section_name); */
/* 		} */
/* 	    } */
/* 	} */
/*     } else if (get_mb_mode() == MB_MODE_CONF_IMAGE) { */
/*         current_section = get_mb_image(); */
/* 	if (strncmp(cmdline[0], "filename", 8) == 0) { */
/* 	    if (cmdline[1]) { */
/* 		MB_DEBUG("updating filename to %s for image %s\n", cmdline[1], current_section); */
/* 		sections = cfg_size(cfg, "image"); */
/* 		m = 0; */
/* 		for (n = 0; n < sections; n++) { */
/* 		    section = cfg_getnsec(cfg, "image", n); */
/* 		    if (strncmp(cfg_title(section), current_section, strlen(cfg_title(section))) == 0) { */
/* 			cfg_setstr(section, "filename", cmdline[1]); */
/* 			m++; */
/* 		    } */
/* 		} */
/* 		if (m) { */
/* 		    printf("%s/filename -> %s [ok]\n", current_section, cmdline[1]); */
/* 		} else { */
/* 		    /\* new image section *\/ */
/* 		    add_section(cfg, "image", current_section); */
/* 		    printf("new image %s added [ok]\n", current_section); */
		    
/* 		    sections = cfg_size(cfg, "image"); */
/* 		    m = 0; */
/* 		    for (n = 0; n < sections; n++) { */
/* 			section = cfg_getnsec(cfg, "image", n); */
/* 			if (strncmp(cfg_title(section), current_section, strlen(cfg_title(section))) == 0) { */
/* 			    cfg_setstr(section, "filename", cmdline[1]); */
/* 			    m++; */
/* 			} */
/* 		    } */
/* 		    if (m) { */
/* 			printf("%s/filename -> %s [ok]\n", current_section, cmdline[1]); */
/* 		    } else { */
/* 			MB_DEBUG("set filename failed\n"); */
/* 		    } */
/* 		} */
/* 	    } */
/* 	} */
/*     } else if (get_mb_mode() == MB_MODE_CONF_NET) { */
/*         current_section = get_mb_network(); */
/* 	if (strncmp(cmdline[0], "address", 7) == 0 ||  */
/* 	    strncmp(cmdline[0], "gateway", 7) == 0 ) { */
/* 	    if (cmdline[1]) { */
		
/* 		sections = cfg_size(cfg, "network"); */
/* 		m = 0; */
/* 		for (n = 0; n < sections; n++) { */
/* 		    section = cfg_getnsec(cfg, "network", n); */
/* 		    if (strncmp(cfg_title(section), current_section, strlen(cfg_title(section))) == 0) { */
/* 			cfg_setstr(section, cmdline[0], cmdline[1]); */
/* 			m++; */
/* 		    } */
/* 		} */
/* 		if (m) { */
/* 		    printf("%s/%s -> %s [ok]\n", current_section, cmdline[0], cmdline[1]); */
/* 		} else { */
/* 		    /\* new network section *\/ */
/* 		    add_section(cfg, "network", current_section); */
/* 		    printf("new network %s added [ok]\n", current_section); */
		    
/* 		    sections = cfg_size(cfg, "network"); */
/* 		    m = 0; */
/* 		    for (n = 0; n < sections; n++) { */
/* 			section = cfg_getnsec(cfg, "network", n); */
/* 			if (strncmp(cfg_title(section), current_section, strlen(cfg_title(section))) == 0) { */
/* 			    cfg_setstr(section, cmdline[0], cmdline[1]); */
/* 			    m++; */
/* 			} */
/* 		    } */
/* 		    if (m) { */
/* 			printf("%s/%s -> %s [ok]\n", current_section, cmdline[0], cmdline[1]); */
/* 		    } else { */
/* 			MB_DEBUG("set %s failed\n", cmdline[0]); */
/* 		    } */
/* 		} */
/* 	    } */
/*	} */
/*    }     */
    return 0;
}

int cmd_write(struct cli_def *cli, cfg_t *cfg, char **cmdline) 
{
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

    //config = get_running_config(cfg); XXX
    config = "";
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

int cmd_shell(struct cli_def *cli, cfg_t *cfg, char **cmdline) {
    	MB_DEBUG("[mb] cmd_shell: starting ash\n");
	do_exec("/bin/ash", "ash", 0);
	MB_DEBUG("[mb] cmd_shell: ash exited\n");
	return 0;
}
