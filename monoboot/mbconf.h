static cfg_opt_t image_opts[] = {
    CFG_STR("filename", 0, CFGF_NONE),
    CFG_END()
};

static cfg_opt_t network_opts[] = {
    CFG_STR("address", 0, CFGF_NONE),
    CFG_STR("gateway", 0, CFGF_NONE),
    CFG_END()
};

static cfg_opt_t opts[] = {
    CFG_INT("version",    0,            CFGF_NONE),
    CFG_INT("delay",      0,            CFGF_NONE),
    CFG_STR("bootonce",   "none",       CFGF_NONE),
    CFG_STR("tryonce",    "none",       CFGF_NONE),
    CFG_STR("default",    "none",       CFGF_NONE),
    CFG_STR("fallback",   "none",       CFGF_NONE),
    CFG_STR("lasttry",    "none",       CFGF_NONE),
    CFG_STR("lastboot",   "none",       CFGF_NONE),
    CFG_STR("bootimage",  "none",       CFGF_NONE),
    CFG_SEC("image",      image_opts,   CFGF_MULTI | CFGF_TITLE),
    CFG_SEC("network",    network_opts, CFGF_MULTI | CFGF_TITLE),
    CFG_STR("line",       "none",       CFGF_LIST),
    CFG_STR("password",   "none",       CFGF_NONE),
    CFG_END()
};
