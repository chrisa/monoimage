#define C99_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <confuse.h>
#include "monoboot.h"

/* $Id$ */

/*
 * get_kernel_console
 * 
 * obtain the console= string from /proc. call this with 
 * /proc already mounted, eh. 
 *
 */
  
char *get_kernel_console(void)
{
    char *p;
    char *console;
    char cmdline[MB_CMDLINE_MAX];
    int fd, len;
    
    fd = open("/proc/cmdline", O_RDONLY);
    if (!fd) {
	MB_DEBUG("[mb] cmd_write: open /proc/cmdline for read failed: %s (/proc not mounted?)\n", strerror(errno));
	return NULL;
    }
    if (read(fd, cmdline, MB_CMDLINE_MAX) < 0) {
	MB_DEBUG("[mb] cmd_write: read from /proc/cmdline failed: %s\n", strerror(errno));
	return NULL;
    }
    close(fd);
    
    /* walk the string until we get console=, or we reach the end. */
    p = cmdline;
    len = strlen(cmdline);
    while (strncmp("console=", p, 8) != 0) {
	if ( (p - cmdline) == (len - 8) )
	    break;
	p++;
    }
	
    /* did we find console= ? */
    if ( (p - cmdline) < len) {
	console = p;

	/* walk until we see space or newline or null */
	while (*p != ' ' && *p != '\n' && *p != '\0')
	    p++;
	
	*p = '\0';
	p = strdup(console);

    } else {

	/* we didn't find it, reset p */
	p = NULL;
    }

    return p;
}
