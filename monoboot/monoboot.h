/* $Id$ */

#include <libcli.h>

/* magic numbers */

#define MB_IMAGE_MAX    32
#define MB_CONFIG_MAX   8000
#define MB_TIMESTR_MAX  200
#define MB_PROMPT_MAX   32
#define MB_NETWORK_MAX  16
#define MB_PATH_MAX     200
#define MB_CMDLINE_MAX  255

#define MB_CM_MAX	32
#define MB_CM_YES	0
#define MB_CM_NO	1
#define MB_CM_FAIL	2

/* paths */

#define MB_PATH_CONFIG  "/config"
#define MB_PATH_IMAGES  "/images"

/* config file names */

#define MB_CONF		"mb.conf"
#define MB_CONF_NEW	"mb.conf.new"
#define MB_CONF_OLD	"mb.conf.old"

/* locations of required binaries */

#define KEXEC_BINARY "/usr/sbin/kexec"
#define TFTP_BINARY  "/usr/bin/tftp"
#define SCP_BINARY   "/usr/bin/scp"
#define IP_BINARY    "/bin/ip"
#define MOUNT_BINARY "/bin/mount"
#define RSYNC_BINARY "/usr/bin/rsync"

/* CLI config modes */

#define MODE_CONFIG_IMAGE 1
#define MODE_CONFIG_NET   2

/* CLI errors */

#define MB_ERR_ARGUMENTS   -1
#define MB_ERR_NOSUCHTAG   -2
#define MB_ERR_FILEMISSING -3
#define MB_ERR_KEXECFAIL   -4

/* cmdline errors */

#define MB_CMDLINE_NOPROC  -1
#define MB_CMDLINE_PARSE   -2



#define MB_DEBUG(...) fprintf(stderr, __VA_ARGS__)

/* main functions */

cfg_t* load_config(char* file);
void show_config(cfg_t *cfg);
void drop_config(cfg_t *cfg);
void save_config(cfg_t *cfg);
int check_last(cfg_t *cfg, char* bootimage);

/* exec functions */

int do_exec (const char*, const char*, ...);
int do_tftp (cfg_t *, char *, char *);
int do_netconf (cfg_t *);
void do_exit (void);

/* CLI functions */

void mb_interact(cfg_t *cfg);
int get_keypress(int delay);
void set_canonical(int flag);

/* extra libconfuse public function */

void add_section(cfg_t *cfg, char *name, char *title);

/* command functions */

int cmd_boot(struct cli_def *, cfg_t *, char **);
int cmd_show_run(struct cli_def *, cfg_t *, char **);
int cmd_show_start(struct cli_def *, cfg_t *, char **);
int cmd_show_ver(struct cli_def *, cfg_t *, char **);
int cmd_show_disk(struct cli_def *, cfg_t *, char **);
int cmd_copy(struct cli_def *, cfg_t *, char **);
int cmd_conf(struct cli_def *, cfg_t *, char **);
int cmd_exit(struct cli_def *, cfg_t *, char **);
int cmd_write(struct cli_def *, cfg_t *, char **);
int cmd_shell(struct cli_def *, cfg_t *, char **);

/* util functions */
char *get_kernel_console(void);
int check_mounted(char*);
