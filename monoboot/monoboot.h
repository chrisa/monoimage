#define MB_CONF		"mb.conf"
#define MB_CONF_NEW	"mb.conf.new"
#define MB_CONF_OLD	"mb.conf.new"

#define KEXEC_BINARY    "kexec"

#define MB_DEBUG(...) fprintf(stderr, __VA_ARGS__)

cfg_t* load_config(char* file);
void show_config(cfg_t *cfg);
void drop_config(cfg_t *cfg);
void save_config(cfg_t *cfg);
int check_last(cfg_t *cfg);
void update_config_for_fallback(cfg_t *cfg);
void boot_image(cfg_t *cfg);
void mb_interact(cfg_t *cfg);
