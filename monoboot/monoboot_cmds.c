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

void cmd_boot(cfg_t *cfg, char **cmdline) {
    
    pid_t pid;
    int n,m,images;
    char *image_tag;
    char image_file[256];
  
    /* if the user said "boot <something>" then check that tag exists,
       then boot it, else default tag */

    if (cmdline[1] != NULL) {
	/* check */
	images = cfg_size(cfg, "image");
	m = 0;
	for (n = 0; n < images; n++) {
	    cfg_t *image = cfg_getnsec(cfg, "image", n);
	    if (strncmp(cfg_title(image), cmdline[1], strlen(cmdline[1])) == 0) {
		m++;
	    }
	}
	if (m > 0) {
	    image_tag = cmdline[1];
	} else {
	    printf("no such tag %s\n", cmdline[1]);
	    return;
	}
    } else {
	image_tag = cfg_getstr(cfg, "default");
    }

    cfg_setstr(cfg, "lasttry", image_tag);

    for (n = 0; n < images; n++) {
	cfg_t *image = cfg_getnsec(cfg, "image", n);
	if (strncmp(cfg_title(image), image_tag, strlen(cfg_title(image))) == 0) {
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


void cmd_show(cfg_t *cfg, char **cmdline) {
    int images, n;

    /* we expect either 'show running-config' or 'show startup-config' */
    if (cmdline[1] == NULL) {
	return;
    } else {
	if (strncmp(cmdline[1], "r", 1) == 0) {
	    /* show run */
	    printf("running-config:\n");
	    printf("version %d\n",  cfg_getint(cfg,"version"));
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

void cmd_conf(cfg_t *cfg, char **cmdline) {
}
