#define C99_SOURCE
#include <stdlib.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <confuse.h>

#include "monoboot.h"
#include "monoboot_cli.h"
#include "monoboot_cmds.h"

/* $Id$ */

/* global pointer for lines read with readline */
char *line_read = (char *)NULL;
/* the current prompt */
char mb_prompt[MB_PROMPT_MAX];
/* the current mode */
int mb_mode;
/* the current image being edited */
char *mb_image;
/* the current network being edited */
char *mb_network;

/* tokenise a command line, return a null terminated list
   of char *s */

char **split_cmdline (char *string) {
    char *cp;
    char **vec = NULL;
    int i = 0;

    if (string == NULL)
	return NULL;
    
    cp = string;

    /* Skip white spaces. */
    while (isspace ((int) *cp) && *cp != '\0')
	cp++;
    
    /* Return if there is only white spaces */
    if (*cp == '\0')
	return NULL;
    
    /* skip a commented line */
    if (*cp == '!' || *cp == '#')
	return NULL;
    
    /* Copy each command piece and set into vector. */
    while (1) {

	if (*cp == '\0') {
	    vec = (char **)realloc(vec, (i + 1) * sizeof(char *));
	    vec[i] = NULL;
	    return vec;
	}

	vec = (char **)realloc(vec, (i + 1) * sizeof(char *));
	if (vec == NULL) {
	    printf("realloc failed: %s\n", strerror(errno));
	    return NULL;
	}

	vec[i] = cp;
	i++;

	while (!(isspace ((int) *cp) || *cp == '\r' || *cp == '\n') &&
	       *cp != '\0')
	    cp++;
	
	while ((isspace ((int) *cp) || *cp == '\n' || *cp == '\r') &&
	       *cp != '\0') {
	    *cp = '\0';
	    cp++;
	}
    }
}

/* free a list allocated for a split cmdline */

void cmdline_free(char **vec) {
    free(vec);
}

/* 
 * Read a string, and return a pointer to it.
 * Returns NULL on EOF. 
 */

char *rl_gets () {
    /* If the buffer has already been allocated,
       return the memory to the free pool. */
    if (line_read) {
	free (line_read);
	line_read = (char *)NULL;
    }
    
    /* Get a line from the user. */
    line_read = readline (mb_prompt);
    
    /* If the line has any text in it,
       save it on the history. */
    if (line_read && *line_read)
	add_history (line_read);
    
    return (line_read);
}

/*
 * set the mbsh prompt
 */

void set_mb_prompt (char *prompt) {
    /* must clobber previous longer prompt with our NULL, hence weird
       + 1 */
    strncpy(mb_prompt, prompt, strlen(prompt) + 1);
}

/* 
 * set up libreadline - 
 *  turn off tabbing of filenames
 *  custom complete?
 */

void init_readline (void) {
    rl_bind_key ('\t', rl_insert);
    set_mb_prompt("mb> ");
}

/* 
 * loop reading commands from the user, until 
 * they either reboot or start a kernel 
 */
void mb_interact(cfg_t *cfg) {
    char **cmdline;
    init_readline();

    while ( (line_read = rl_gets()) != NULL) {
	cmdline = split_cmdline(line_read);

	if (cmdline) {

	    /* == EXIT == */
	    if ( strncmp(cmdline[0], "exit", 4) == 0 ) {
		cmd_exit(cfg, cmdline);
	    }

	    if (get_mb_mode() == MB_MODE_CONF || 
		get_mb_mode() == MB_MODE_CONF_IMAGE ||
		get_mb_mode() == MB_MODE_CONF_NET ) {

		/* hand all conf mode commands to cmd_conf */
		cmd_conf(cfg, cmdline);

	    } else if (get_mb_mode() == MB_MODE_EXEC) {
	  
		/* == BOOT == */
		if ( strncmp(cmdline[0], "boot", 4) == 0 ) {
		    cmd_boot(cfg, cmdline);
		}

		/* == SHOW == */
		if ( strncmp(cmdline[0], "show", 4) == 0 ) {
		    cmd_show(cfg, cmdline);
		}

		/* == COPY == */
		if ( strncmp(cmdline[0], "copy", 4) == 0 ) {
		    // cmd_copy(cfg, cmdline);
		}

		/* == CONF == */
		if ( strncmp(cmdline[0], "conf", 4) == 0 ) {
		    cmd_conf(cfg, cmdline);
		}

		/* == WRITE == */
		if ( strncmp(cmdline[0], "write", 5) == 0 ) {
		    cmd_write(cfg, cmdline);
		}

	    } else {
		/* unknown mode */
	    }
	}
	cmdline_free(cmdline);
    }
    MB_DEBUG("\n[mb] mb_interact: exiting mb_interact\n");
}


/* cli mode get/set */
void set_mb_mode(int mode) {
    mb_mode = mode;
}
int get_mb_mode(void) {
    return mb_mode;
}

/* cli current-image get/set */
void set_mb_image(char *image) {
    if (mb_image == NULL)
	mb_image = (char *)malloc(sizeof(char) * MB_IMAGE_MAX);
    strncpy(mb_image, image, strlen(image));
    mb_image[strlen(image)] = '\0';
}
char *get_mb_image(void) {
    return mb_image;
}

/* cli current-network get/set */
void set_mb_network(char *network) {
    if (mb_network == NULL)
	mb_network = (char *)malloc(sizeof(char) * MB_NETWORK_MAX);
    strncpy(mb_network, network, strlen(network));
    mb_network[strlen(network)] = '\0';
}
char *get_mb_network(void) {
    return mb_network;
}
