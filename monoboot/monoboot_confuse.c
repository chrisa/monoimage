/*
 * these two functions are ripped off from confuse.c ....
 * for some reason there's no public function to add a 
 * section to the in-memory config
 * ...
 */

#define _GNU_SOURCE
#include <string.h>
#include <malloc.h>
#include <confuse.h>
  
static cfg_opt_t *cfg_dupopts(cfg_opt_t *opts)
{
	int n;
	cfg_opt_t *dupopts;

	for(n = 0; opts[n].name; n++)
		/* do nothing */ ;
	dupopts = (cfg_opt_t *)malloc(++n * sizeof(cfg_opt_t));
	memcpy(dupopts, opts, n * sizeof(cfg_opt_t));
	return dupopts;
}

void add_section(cfg_t *cfg, char *name, char *title) {
    cfg_opt_t *opt;
    cfg_value_t *val;
    int i;
    
    for (i = 0; cfg->opts[i].type != CFGT_NONE; i++) {
	if (strncmp(cfg->opts[i].name, name, strlen(name)) == 0) {
	    opt = &cfg->opts[i];
	}
    }
    opt->values = (cfg_value_t **)realloc(opt->values, (opt->nvalues+1) * sizeof(cfg_value_t *));
    opt->values[opt->nvalues] = (cfg_value_t *)malloc(sizeof(cfg_value_t));
    memset(opt->values[opt->nvalues], 0, sizeof(cfg_value_t));
    val = opt->values[opt->nvalues++];
    
    cfg_free(val->section);
    val->section = (cfg_t *)malloc(sizeof(cfg_t));
    memset(val->section, 0, sizeof(cfg_t));
    val->section->opts = cfg_dupopts(opt->subopts);
    val->section->flags = cfg->flags;
    val->section->filename = cfg->filename ? strdup(cfg->filename) : 0;
    val->section->line = cfg->line;
    val->section->errfunc = cfg->errfunc;
    val->section->name = strdup(opt->name);
    val->section->title = strndup(title, strlen(title));
}

