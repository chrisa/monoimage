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

#define MAXARGS 31

int do_exec (const char *binary, const char *args, ...) {
    pid_t pid, status;
    va_list ap;
    char *array[MAXARGS +1];
    int argno = 0;

    MB_DEBUG("exec %s ( ", binary);

    va_start(ap, args);
    while (args != 0 && argno < MAXARGS)
    {
	MB_DEBUG("%s ", args);
        array[argno++] = (char *)args;
        args = va_arg(ap, const char *);
    }
    array[argno] = (char *) 0;
    va_end(ap);

    MB_DEBUG(")\n");

    if ( (pid = fork()) < 0) {
	MB_DEBUG("[mb] fork failed: %s\n", strerror(errno));
	return 1;
    } else if (pid == 0) {
	if (execvp(binary, array) < 0) {
	    MB_DEBUG("[mb] exec of %s failed: %s\n", binary, strerror(errno));
	    exit(1);
	}
    }
    if (waitpid(pid, &status, 0) < 0) {
	MB_DEBUG("[mb] wait failed: %s\n", strerror(errno));
	return 1;
    } else if (WIFEXITED(status)) {
	if (WEXITSTATUS(status)) {
	    MB_DEBUG("[mb] child exited non-zero\n");
	    return 1;
	} else {
	    MB_DEBUG("[mb] child exited zero, looks good\n");
	    return 0;
	}
    } else {
	MB_DEBUG("[mb] child died\n");
	return 1;
    }
}

int do_tftp (cfg_t *cfg, char *src, char *dst) {
    char *cmd;
    char *p;
    char *host = NULL;
    char *file = NULL;
    
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

    return do_exec(TFTP_BINARY, "tftp", host, "-v", "-c", cmd, file, (char *) 0);
}

int do_netconf(cfg_t *cfg) {
    
    int net_count, n, ret;
    const char *iface;
    char *addr, *gw;

    /*
     * this will do:
     * 
     * ip address flush dev %s
     * ip address add %s dev %s
     * ip link set %s up
     * ip route del default
     * ip route add default via %s
     * 
     */
    
    net_count = cfg_size(cfg, "network");
    for (n = 0; n < net_count; n++) {
	cfg_t *network = cfg_getnsec(cfg, "network", n);
	iface = cfg_title(network);
	addr = cfg_getstr(network, "address");
	gw = cfg_getstr(network, "gateway");
	
	ret += do_exec(IP_BINARY, "ip", "address", "flush", "dev", iface, 0);
	ret += do_exec(IP_BINARY, "ip", "address", "add", addr, "dev", iface, 0);
	ret += do_exec(IP_BINARY, "ip", "link", "set", "eth0", "up", 0);
	ret += do_exec(IP_BINARY, "ip", "route", "del", "default", 0);
	ret += do_exec(IP_BINARY, "ip", "route", "add", "default", "via", gw, 0);
    }
    return ret;
}

/* 
 * do_exit - add cleanup here (umounts? unconfig network?). 
 */

void do_exit(void) {
    MB_DEBUG("[mb] exiting cleanly...\n");
    exit(0);
}
