#define C99_SOURCE
#define _GNU_SOURCE
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
#include "monoboot_exec.h"

int do_tftp (cfg_t *cfg, char *src, char *dst) {
    char *cmd;
    char *p;
    char *host = NULL;
    char *file = NULL;
    pid_t pid, status;
    
    if (strncmp(src, "tftp:/", 6) == 0 && strncmp(dst, "disk:/", 6) == 0) {
	cmd = strdup("get");
	p = src;
	while (*p != '/') 
	    p++;
	while (*p == '/') 
	    p++;
	host = p;
	while (*p != '/') 
	    p++;
	*p = '\0'; p++;
	while (*p == '/') 
	    p++;
	file = p;

    } else if (strncmp(dst, "tftp:/", 6) == 0 && strncmp(src, "disk:/", 6) == 0) {
	cmd = strdup("put");
	p = dst;
	while (*p != '/') 
	    p++;
	while (*p == '/') 
	    p++;
	host = p;
	while (*p != '/') 
	    p++;
	*p = '\0';

	p = src;
	while (*p != '/') 
	    p++;
	*p = '\0'; p++;
	while (*p == '/') 
	    p++;
	file = p;
	
    } else {
	printf("can't parse copy command\n");
	return 1;
    }
    
    if ( (pid = fork()) < 0) {
	MB_DEBUG("[mb] do_tftp: fork failed: %s\n", strerror(errno));
	return 1;
    } else if (pid == 0) {
	if (execlp(TFTP_BINARY, "tftp", host, "-v", "-c", cmd, file, (char *) 0) < 0) {
	    MB_DEBUG("[mb] do_tftp: exec of %s failed: %s\n", TFTP_BINARY, strerror(errno));
	    exit(1);
	}
    }
    if (waitpid(pid, &status, 0) < 0) {
	MB_DEBUG("[mb] do_tftp: wait failed: %s\n", strerror(errno));
	return 1;
    } else if (WIFEXITED(status)) {
	MB_DEBUG("[mb] child exited ... \n");
	if (WEXITSTATUS(status)) {
	    MB_DEBUG("[mb] ... non-zero\n");
	    return 1;
	} else {
	    MB_DEBUG("[mb] ... zero, looks good\n");
	    return 0;
	}
    } else {
	MB_DEBUG("[mb] child died\n");
	return 1;
    }
}
