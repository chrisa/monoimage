#ifndef BZIMAGE_LOADER_H
#define BZIMAGE_LOADER_H

#include "../../kexec.h"

int do_bzImage_load(struct kexec_info *info,
                    const char *kernel, off_t kernel_len,
                    const char *command_line, off_t command_line_len,
                    const char *initrd, off_t initrd_len,
                    int real_mode_entry, int debug);

#endif /* BZIMAGE_LOADER_H */
