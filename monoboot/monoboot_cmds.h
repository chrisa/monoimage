/*
 *
 * monoboot_cmds.h -- functions for the mb
 * shell's commands
 *
 */

/*
 * boot
 * show
 * copy
 * conf
 */

void cmd_boot(cfg_t *, char **);
void cmd_show(cfg_t *, char **);
void cmd_copy(cfg_t *, char **);
void cmd_conf(cfg_t *, char **);
void cmd_exit(cfg_t *, char **);

/* needs these binaries */

#define KEXEC_BINARY "/usr/sbin/kexec"
#define TFTP_BINARY  "/usr/bin/tftp"
#define SCP_BINARY   "/usr/bin/scp"
#define IP_BINARY    "/bin/ip"

/* CLI modes */

#define MB_MODE_EXEC 0
#define MB_MODE_CONF 1
