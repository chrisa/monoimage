#define C99_SOURCE
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <confuse.h>
#include "monoboot.h"
#include "monoboot_cmds.h"
#include "monoboot_cli.h"

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
	image_tag = cfg_getstr(cfg, "default");
    }

    MB_DEBUG("[mb] image_tag: %s\n", image_tag);

    cfg_setstr(cfg, "lasttry", image_tag);

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
    save_config(cfg);
    drop_config(cfg);

    /* now exec the kexec -e -- hardly worth forking */
    execlp("KEXEC_BINARY", "kexec", "-e", (char *) 0);

    /* can't happen; we're off booting the new kernel (right?) */
    MB_DEBUG("[mb] boot_image: we're still here. exec of %s failed: %s\n", KEXEC_BINARY, strerror(errno));
    return;
}


void cmd_show(cfg_t *cfg, char **cmdline) {
    int images, n;

    /* we expect either 'show running-config' or 'show startup-config' */
    if (cmdline[1] == NULL) {
	return;
    } else {
	if (strncmp(cmdline[1], "r", 1) == 0) {
	    /* show run */
	    printf("\n");
	    printf("version %ld\n",  cfg_getint(cfg,"version"));
	    printf("bootonce %s\n", cfg_getstr(cfg,"bootonce"));
	    printf("fallback %s\n", cfg_getstr(cfg,"fallback"));
	    printf("lastboot %s\n", cfg_getstr(cfg,"lastboot"));
	    printf("tryonce %s\n",  cfg_getstr(cfg,"tryonce"));
	    printf("lasttry %s\n",  cfg_getstr(cfg,"lasttry"));

	    images = cfg_size(cfg, "image");
	    for (n = 0; n < images; n++) {
		cfg_t *image = cfg_getnsec(cfg, "image", n);
		printf("\nimage %s {\n", cfg_title(image));
		printf("  filename %s\n", cfg_getstr(image, "filename"));
		printf("}\n");
	    }
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
    int images, n;

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
		for (n = 0; n < images; n++) {
		    image = cfg_getnsec(cfg, "image", n);
		    if (strncmp(cfg_title(image), current_image, strlen(cfg_title(image))) == 0) {
			cfg_setstr(image, "filename", cmdline[1]);
		    }
		}
		printf("%s -> %s [ok]\n", "filename", cmdline[1]);
	    }
	}
    }    
}
