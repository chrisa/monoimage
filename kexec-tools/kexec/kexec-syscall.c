#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include "kexec.h"
#define __LIBRARY__
#include <syscall.h>

#define	LINUX_REBOOT_MAGIC1	0xfee1dead
#define	LINUX_REBOOT_MAGIC2	672274793
#define	LINUX_REBOOT_MAGIC2A	85072278
#define	LINUX_REBOOT_MAGIC2B	369367448

#define	LINUX_REBOOT_CMD_RESTART	0x01234567
#define	LINUX_REBOOT_CMD_HALT		0xCDEF0123
#define	LINUX_REBOOT_CMD_CAD_ON		0x89ABCDEF
#define	LINUX_REBOOT_CMD_CAD_OFF	0x00000000
#define	LINUX_REBOOT_CMD_POWER_OFF	0x4321FEDC
#define	LINUX_REBOOT_CMD_RESTART2	0xA1B2C3D4
#define LINUX_REBOOT_CMD_EXEC_KERNEL    0x18273645
#define LINUX_REBOOT_CMD_KEXEC_OLD	0x81726354
#define LINUX_REBOOT_CMD_KEXEC_OLD2	0x18263645
#define LINUX_REBOOT_CMD_KEXEC		0x45584543


#define __NR_kexec_load 259
_syscall4(int, reboot, int, magic1, int, magic2, int, cmd, void*, arg);
_syscall4(long, kexec_load, void *, entry, unsigned long, nr_segments, struct kexec_segment *, segments, unsigned long, flags);

#if 0
long kexec(void *entry, unsigned long nr_segments, struct kexec_segment *segments)
{
	long result;
	result = kexec_load(entry, nr_segments, segments, 0);
	if (result != 0)
		return result;
	return reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_KEXEC, 0);
}
#else
long kexec(void)
{
	return reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_KEXEC, 0);
}
#endif

