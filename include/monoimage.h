/*
 * monoimage format:
 * 
 * [header][kernel][initrd][rootfs]
 *
 */

struct monoimage_header {
	uint8_t   magic[2];
	uint16_t  version;
	uint8_t   runfrom;
	uint8_t   fstype;
	uint32_t  kernel_offset;
	uint32_t  initrd_offset;
	uint32_t  rootfs_offset;
};

#define MI_MAGIC "MI"

#define MI_RUNFROM_LOOP  0
#define MI_RUNFROM_CLOOP 1
#define MI_RUNFROM_TMPFS 2
#define MI_RUNFROM_NFS   3

#define MI_FSTYPE_EXT2     0
#define MI_FSTYPE_SQUASHFS 1
#define MI_FSTYPE_TAR      2
#define MI_FSTYPE_TARGZ    3
#define MI_FSTYPE_CPIO     4
#define MI_FSTYPE_CPIOGZ   5

