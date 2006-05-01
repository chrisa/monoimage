/* get-uuid.c -- tiny test program to print the UUID of a filesystem 
 *
 * build: gcc -g -o get-uuid get-uuid.c -lext2fs
 *
 * requires libext2fs (debian: e2fslibs-dev)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2_types.h>
#include <ext2fs/ext2fs.h>

struct uuid {
	__u32	time_low;
	__u16	time_mid;
	__u16	time_hi_and_version;
	__u16	clock_seq;
	__u8	node[6];
};

/* Returns 1 if the uuid is the NULL uuid */
int is_null_uuid(void *uu)
{
	__u8 	*cp;
	int	i;

	for (i=0, cp = uu; i < 16; i++)
		if (*cp)
			return 0;
	return 1;
}

static void unpack_uuid(void *in, struct uuid *uu)
{
	__u8	*ptr = in;
	__u32	tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	tmp = (tmp << 8) | *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_low = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_mid = tmp;
	
	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_hi_and_version = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->clock_seq = tmp;

	memcpy(uu->node, ptr, 6);
}

void uuid_to_str(void *uu, char *out)
{
	struct uuid uuid;

	unpack_uuid(uu, &uuid);
	sprintf(out,
		"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		uuid.time_low, uuid.time_mid, uuid.time_hi_and_version,
		uuid.clock_seq >> 8, uuid.clock_seq & 0xFF,
		uuid.node[0], uuid.node[1], uuid.node[2],
		uuid.node[3], uuid.node[4], uuid.node[5]);
}

const char *uuid2str(void *uu)
{
	static char buf[80];

	if (is_null_uuid(uu))
		return "<none>";
	uuid_to_str(uu, buf);
	return buf;
}

int main (int argc, char **argv)
{
  char *device;
  errcode_t e2fserr;
  ext2_filsys fs;
  
  device = argv[1];
  e2fserr = ext2fs_open(device, 0, 0, 0, unix_io_manager, &fs);

  fprintf(stderr, "%s\n", uuid2str(fs->super->s_uuid));

  e2fserr = ext2fs_close(fs);
  return 0;
}

