/* $Id$ */

#define MB_PROMPT_MAX 32
#define MB_NETWORK_MAX 16

void mb_interact(cfg_t *cfg);

void set_mb_mode(int);
int get_mb_mode(void);

void set_mb_image(char *);
char *get_mb_image(void);
void set_mb_network(char *);
char *get_mb_network(void);

void set_mb_prompt(char *);

/* CLI modes */

#define MB_MODE_EXEC       0
#define MB_MODE_CONF       1
#define MB_MODE_CONF_IMAGE 2
#define MB_MODE_CONF_NET   3
