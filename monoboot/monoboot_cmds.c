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

void cmd_boot(cfg_t *cfg, char **cmdline) {
    
    pid_t pid;
    int n,images;
    char image_file[256];
  
    cfg_setstr(cfg, "lasttry", cfg_getstr(cfg, "default"));

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

