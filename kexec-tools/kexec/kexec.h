#ifndef KEXEC_H
#define KEXEC_H

#include <stdint.h>
#include <x86/x86-linux.h>

struct kexec_segment {
	void *buf;
	size_t bufsz;
	void *mem;
	size_t memsz;
};

void usage(void);
long kexec(void);
long kexec_load(void *entry, unsigned long nr_segments, struct kexec_segment *segments, unsigned long);

struct memory_range {
	unsigned long long start, end;
	unsigned type;
#define RANGE_RAM	0
#define RANGE_RESERVED	1
#define RANGE_ACPI	2
#define RANGE_ACPI_NVS	3
};

int get_memory_ranges(struct memory_range **range, int *ranges);
int valid_memory_range(struct kexec_segment *segment);
int sort_segments(struct kexec_segment *segment, int segments);
int locate_arguments(struct kexec_segment *segment, int segments, 
	size_t align, unsigned long max);

int setup_linux_parameters(struct x86_linux_param_header *real_mode);


struct file_type {
	const char *name;
	int (*probe)(FILE *file);
	int (*load)(FILE *file, int argc, char **argv,
		void **entry, struct kexec_segment **segments, int *nr_segments);
	void (*usage)(void);
};

extern struct file_type file_type[];
extern int file_types;

#define OPT_HELP		'h'
#define OPT_VERSION		'v'
#define OPT_DEBUG		'd'
#define OPT_FORCE		'f'
#define OPT_EXEC		'e'
#define OPT_LOAD		'l'
#define OPT_TYPE		't'
#define OPT_MAX			256
#define KEXEC_OPTIONS \
	{ "help",		0, 0, OPT_HELP }, \
	{ "version",		0, 0, OPT_VERSION }, \
	{ "force",		0, 0, OPT_FORCE }, \
	{ "load",		0, 0, OPT_LOAD }, \
	{ "exec",		0, 0, OPT_EXEC }, \
	{ "type",		1, 0, OPT_TYPE }, 
#define KEXEC_OPT_STR "hvdfelt:"

#endif /* KEXEC_H */
