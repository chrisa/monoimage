#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include "kexec.h"


/* local variables */
static struct memory_range *memory_range;
static int memory_ranges;

int valid_memory_range(struct kexec_segment *segment)
{
	int i;
	for(i = 0; i < memory_ranges; i++) {
		unsigned long mstart, mend;
		unsigned long sstart, send;
		/* Only consider memory ranges */
		if (memory_range[i].type != RANGE_RAM)
			continue;
		mstart = memory_range[i].start;
		mend = memory_range[i].end;
		sstart = (unsigned long)segment->mem;
		send = sstart + segment->memsz -1;
		/* Check to see if we are fully contained */
		if ((mstart <= sstart) && (mend >= send)) {
			return 0;
		}
	}
	return -1;
}

int sort_segments(struct kexec_segment *segment, int segments)
{
	int i, j;
	void *end;
	/* Do a stupid insertion sort... */
	for(i = 1; i < segments; i++) {
		int temp_idx;
		struct kexec_segment temp;
		temp_idx = i;
		for(j = i +1; j < segments; j++) {
			if (segment[j].mem < segment[temp_idx].mem) {
				temp_idx = j;
			}
		}
		if (temp_idx != i) {
			temp = segment[temp_idx];
			segment[temp_idx] = segment[i];
			segment[i] = temp;
		}
	}
	/* Now see if any of the segments overlap */
	end = 0;
	for(i = 1; i < segments; i++) {
		if (end > segment[i].mem) {
			fprintf(stderr, "Overlapping memory segments at %p\n",
				end);
			return -1;
		}
		end = ((char *)segment[i].mem) + segment[i].memsz;
	}
	return 0;
}

int locate_arguments(struct kexec_segment *segment, int segments,
	size_t align, unsigned long max)
{
	int i, j;
	struct memory_range *mem_range;
	int max_mem_ranges, mem_ranges;

	/* Compute the free memory ranges */
	max_mem_ranges = memory_ranges + (segments -1);
	mem_range = malloc(max_mem_ranges *sizeof(struct memory_range));
	mem_ranges = 0;
		
	/* Perform a merge on the 2 sorted lists of memory ranges  */
	for(j = 1, i = 0; i < memory_ranges; i++) {
		unsigned long sstart, send;
		unsigned long mstart, mend;
		mstart = memory_range[i].start;
		mend = memory_range[i].end;
		if (memory_range[i].type != RANGE_RAM)
			continue;
		while((j < segments) && (((unsigned long)segment[j].mem) <= mend)) {
			sstart = (unsigned long)segment[j].mem;
			send = sstart + segment[j].memsz -1;
			if (mstart < sstart) {
				mem_range[mem_ranges].start = mstart;
				mem_range[mem_ranges].end = sstart -1;
				mem_range[mem_ranges].type = RANGE_RAM;
				mem_ranges++;
			}
			mstart = send +1;
			j++;
		}
		if (mstart <= mend) {
			mem_range[mem_ranges].start = mstart;
			mem_range[mem_ranges].end = mend;
			mem_range[mem_ranges].type = RANGE_RAM;
			mem_ranges++;
		}
	}
	/* Now find the first memory_range I can use */
	for(i = 0; i < mem_ranges; i++) {
		unsigned long start, size;
		start = (mem_range[i].start + align -1) & ~(align -1);
		size = mem_range[i].end - start;
		if (size >= segment[0].memsz) {
			break;
		}
	}
	if (i == mem_ranges) {
		fprintf(stderr, "Could not find a free area of memory for the arguemnts...\n");
		return -1;
	}
	if (mem_range[i].end > max) {
		fprintf(stderr, "Could not find a free area of memory below: %lx...\n",
			max);
		return -1;
	}
	segment[0].mem = (void *)(unsigned long)((mem_range[i].start + align - 1) & ~(align -1));
	return 0;
}

/*
 *	Load the new kernel
 */
static int my_load(const char *type, int fileind, int argc, char **argv)
{
	char *kernel;
	FILE *fp_kernel;
	int i;
	int result;
	struct kexec_segment *segments;
	int nr_segments;
	void *entry;

	result = 0;
	if (argc - fileind <= 0) {
		fprintf(stderr, "No kernel specified\n");
		usage();
		return -1;
	}
	kernel = argv[fileind];
	fp_kernel = fopen(kernel, "rb");
	if (!fp_kernel) {
		fprintf(stderr, "Cannot open %s: %s\n",
			kernel, strerror(errno));
		return -1;
	}
	if (get_memory_ranges(&memory_range, &memory_ranges) < 0) {
		fprintf(stderr, "Could not get memory layout\n");
		return -1;
	}
	for(i = 0; i < file_types; i++) {
		if (type && (strcmp(type, file_type[i].name) != 0)) {
			break;
		}
		if (file_type[i].probe(fp_kernel) > 0) {
			break;
		}
	}
	if (i == file_types) {
		fprintf(stderr, "Can not determine the file type of %s\n",
			kernel);
		return -1;
	}
	if (file_type[i].load(fp_kernel, argc, argv,
		    &entry, &segments, &nr_segments) < 0) {
		fprintf(stderr, "Can not load %s\n", kernel);
		return -1;
	}
	/* Verify all of the segments load to a valid location in memory */
	for(i = 1; i < nr_segments; i++) {
		if (valid_memory_range(segments +i) < 0) {
			fprintf(stderr, "Invalid memory segment %p - %p\n",
				segments[i].mem,
				((char *)segments[i].mem) + segments[i].memsz);
			return -1;
		}
	}
	/* Sort the segments and verify we don't have overlaps */
	if (sort_segments(segments, nr_segments) < 0) {
		return -1;
	}
	result = kexec_load(entry, nr_segments, segments, 0);
	if (result != 0) {
		/* The load failed, print some debugging information */
		fprintf(stderr, "kexec_load failed: %s\n",
			strerror(errno));
		fprintf(stderr, "entry       = %p\n", entry);
		fprintf(stderr, "nr_segments = %d\n", nr_segments);
		for(i = 0; i < nr_segments; i++) {
			fprintf(stderr, "segment[%d].buf   = %p\n", i, segments[i].buf);
			fprintf(stderr, "segment[%d].bufsz = %x\n", i, segments[i].bufsz);
			fprintf(stderr, "segment[%d].mem   = %p\n", i, segments[i].mem);
			fprintf(stderr, "segment[%d].memsz = %x\n", i, segments[i].memsz);
		}
	}
	return result;
}

/*
 *	Start a reboot.
 */
static int my_shutdown(void)
{
	char *args[8];
	int i = 0;

	args[i++] = "shutdown";
	args[i++] = "-r";
	args[i++] = "now";
	args[i++] = NULL;

	execv("/sbin/shutdown", args);
	execv("/etc/shutdown", args);
	execv("/bin/shutdown", args);

	perror("shutdown");
	return -1;
}
/*
 *	Exec the new kernel
 */
static int my_exec(void)
{
	int result = kexec();
	result = kexec();
	/* I have failed if I make it here */
	fprintf(stderr, "kexec failed: %s\n", 
		strerror(errno));
	return -1;
}

static void version(void)
{
	printf("kexec " VERSION " released " RELEASE_DATE "\n");
}

void usage(void)
{
	int i;
	version();
	printf(
		"Usage: kexec [OPTION]... kernel\n"
		"Directly reboot into a new kernel\n"
		"\n"
		" -h, --help        Print this help.\n"
		" -v, --version     Print the version of kexec.\n"
		" -f, --force       Force an immediate kexec, don't call shutdown.\n"
		" -l, --load        Just load the new kernel into the current kernel.\n"
		" -e, --exec        Just execute a currently loaded kernel.\n"
		" -t, --type=TYPE   Specify the new kernel is of this type.\n"
		"\n"
		"Supported kernel types: \n"
		);
	for(i = 0; i < file_types; i++) {
		printf("%s\n", file_type[i].name);
		file_type[i].usage();
	}
	printf("\n");
	
}

int main(int argc, char *argv[])
{
	int do_load, do_exec, do_shutdown, do_sync, do_ifdown;
	char *type;
	int opt;
	int result, fileind;

	static  const struct option options[] = {
		KEXEC_OPTIONS
		{ 0, 0, 0, 0},
	};
	static const char short_options[] = KEXEC_OPT_STR;
	do_load = 1;
	do_shutdown = 1;
	do_sync = 1;
	do_ifdown = 0;
	do_exec = 0;
	result = 0;
	type = 0;

	opterr = 0; /* Don't complain about unrecognized options here */
	while ((opt = getopt_long(argc, argv, short_options, options, 0)) != -1) {
		switch(opt) {
		case OPT_HELP:
			usage();
			return 0;
		case OPT_VERSION:
			version();
			return 0;
		case OPT_FORCE:
			do_load = 1;
			do_shutdown = 0;
			do_sync = 1;
			do_ifdown = 1;
			do_exec = 1;
			break;
		case OPT_LOAD:
			do_load = 1;
			do_exec = 0;
			do_shutdown = 0;
			break;
		case OPT_EXEC:
			do_load = 0;
			do_shutdown = 0;
			do_sync = 1;
			do_ifdown = 1;
			do_exec = 1;
			break;
		case OPT_TYPE:
			type = optarg;
			break;
		default:
			break;
		}
	}
	fileind = optind;
	/* Reset getopt for the next pass */
	opterr = 1;
	optind = 1;
	if (do_load) {
		result = my_load(type, fileind, argc, argv);
	}
	if ((result == 0) && do_shutdown) {
		result = my_shutdown();
	}
	if ((result == 0) && do_sync) {
		sync();
	}
	if (do_ifdown) {
		extern int ifdown(void);
		(void)ifdown();
	}
	if ((result == 0) && do_exec) {
		result = my_exec();
	}
	fflush(stdout);
	fflush(stderr);
	return result;
} 
