#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include "kexec.h"
#include "kexec-x86.h"

#define MAX_MEMORY_RANGES 64
#define MAX_LINE 160
static struct memory_range memory_range[MAX_MEMORY_RANGES];

/* Return a sorted list of memory ranges. */
int get_memory_ranges(struct memory_range **range, int *ranges)
{
	const char iomem[]= "/proc/iomem";
	int memory_ranges = 0;
	char line[MAX_LINE];
	FILE *fp;
	fp = fopen(iomem, "r");
	if (!fp) {
		fprintf(stderr, "Cannot open %s: %s\n", 
			iomem, strerror(errno));
		return -1;
	}
	while(fgets(line, sizeof(line), fp) != 0) {
		unsigned long long start, end;
		char *str;
		int type;
		int consumed;
		int count;
		if (memory_ranges >= MAX_MEMORY_RANGES)
			break;
		count = sscanf(line, "%Lx-%Lx : %n",
			&start, &end, &consumed);
		if (count != 2) 
			continue;
		str = line + consumed;
		end = end + 1;
#if 0
		printf("%016Lx-%016Lx : %s\n",
			start, end, str);
#endif
		if (memcmp(str, "System RAM\n", 11) == 0) {
			type = RANGE_RAM;
		} 
		else if (memcmp(str, "reserved\n", 9) == 0) {
			type = RANGE_RESERVED;
		}
		else if (memcmp(str, "ACPI Tables\n", 12) == 0) {
			type = RANGE_ACPI;
		}
		else if (memcmp(str, "ACPI Non-volatile Storage\n", 26) == 0) {
			type = RANGE_ACPI_NVS;
		}
		else {
			continue;
		}
		memory_range[memory_ranges].start = start;
		memory_range[memory_ranges].end = end;
		memory_range[memory_ranges].type = type;
#if 0
		printf("%016Lx-%016Lx : %x\n",
			start, end, type);
#endif
		memory_ranges++;
	}
	fclose(fp);
	*range = memory_range;
	*ranges = memory_ranges;
	return 0;
}

struct file_type file_type[] = {
	{ "elf32-x86", elf32_x86_probe, elf32_x86_load, elf32_x86_usage },
	{ "bzImage", bzImage_probe, bzImage_load, bzImage_usage },
};
int file_types = sizeof(file_type)/sizeof(file_type[0]);
