/* for ancient systems use "unsigned short" */

#include <linux/posix_types.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < 132608
#error you need kernel 2.6 headers
#else
#define __linuxrc_dev_t __kernel_old_dev_t
#endif
