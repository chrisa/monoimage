/*
 *
 * monoboot_cmds.h -- functions for the mb
 * shell's commands
 *
 */

/* $Id$ */

/*
 * boot
 * show
 * copy
 * conf
 * write
 */

void cmd_boot(cfg_t *, char **);
void cmd_show(cfg_t *, char **);
void cmd_copy(cfg_t *, char **);
void cmd_conf(cfg_t *, char **);
void cmd_exit(cfg_t *, char **);
void cmd_write(cfg_t *, char **);

