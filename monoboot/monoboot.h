/* $Id$ */

/* magic numbers */

#define MB_IMAGE_MAX    32
#define MB_CONFIG_MAX   8000
#define MB_TIMESTR_MAX  200
#define MB_PROMPT_MAX   32
#define MB_NETWORK_MAX  16

/* config file locations */

#define MB_CONF		"mb.conf"
#define MB_CONF_NEW	"mb.conf.new"
#define MB_CONF_OLD	"mb.conf.old"

/* locations of required binaries */

#define KEXEC_BINARY "/usr/sbin/kexec"
#define TFTP_BINARY  "/usr/bin/tftp"
#define SCP_BINARY   "/usr/bin/scp"
#define IP_BINARY    "/bin/ip"
#define MOUNT_BINARY "/bin/mount"

/* CLI modes */

#define MB_MODE_EXEC       0
#define MB_MODE_CONF       1
#define MB_MODE_CONF_IMAGE 2
#define MB_MODE_CONF_NET   3

#define MB_DEBUG(...) fprintf(stderr, __VA_ARGS__)

/* main functions */

cfg_t* load_config(char* file);
void show_config(cfg_t *cfg);
void drop_config(cfg_t *cfg);
void save_config(cfg_t *cfg);
int check_last(cfg_t *cfg);

/* exec functions */

int do_exec (const char*, const char*, ...);
int do_tftp (cfg_t *, char *, char *);
int do_netconf (cfg_t *);

/* CLI functions */

void mb_interact(cfg_t *cfg);
void set_mb_mode(int);
int get_mb_mode(void);
void set_mb_image(char *);
char *get_mb_image(void);
void set_mb_network(char *);
char *get_mb_network(void);
void set_mb_prompt(char *);


/* command functions */

void cmd_boot(cfg_t *, char **);
void cmd_show(cfg_t *, char **);
void cmd_copy(cfg_t *, char **);
void cmd_conf(cfg_t *, char **);
void cmd_exit(cfg_t *, char **);
void cmd_write(cfg_t *, char **);

