/*
 * Copyright (C) 2012 Žilvinas Valinskas
 * See LICENSE for more information.
 */
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mmio.h"
#include "utils.h"

static const char *usage =
"mmio [options] <address>[@<range>] (1)\n"
"mmio [options] <address> <value>   (2)\n"
"\n"
"OPTIONS:\n"
" -a   - 8bit hex + ascii output\n"
" -b   - 8bit hex output\n"
" -h   - 16bit hex output\n"
" -x   - 32bit hex output\n"
" -k   - use /dev/kmem instead of /dev/mem (default)\n"
"\n"
"(1) dumps specified memory range\n"
"(2) writes specified value to address\n";

void mmio_hexdump(const struct mmio_options *mo, size_t flags)
{
	__hexdump(mo->iobase + mo->offset,
		  mo->iomem + mo->offset,
		  mo->range * 4, flags);
}

static void mmio_normalize(struct mmio_options *mo)
{
	int npages = 0;

	mo->iobase += mo->offset;
	mo->offset = mo->iobase & (getpagesize() - 1);
	mo->iobase = mo->iobase & ~(getpagesize() - 1);

	npages += (mo->range * sizeof(uint32_t)) / getpagesize();
	npages += 1;
	mo->iosize = npages * getpagesize();
}

void mmio_init(struct mmio_options *mo)
{
	char *device;
	int iofd;

	if (!mo->kmem)
		device = "/dev/mem";
	else
		device = "/dev/kmem";

	iofd = open(device, O_RDWR);
	if (iofd < 0)
		die_errno("open() failed");

	mo->iomem = mmap(NULL, mo->iosize,
			 PROT_READ|PROT_WRITE,
			 MAP_SHARED,
			 iofd, mo->iobase);

	if (mo->iomem == MAP_FAILED)
		die_errno("can't map @ %0lX", mo->iobase);

	close(iofd);
}

int mmio_map(struct mmio_options *mo, unsigned long base, size_t length)
{
	mo->iobase = base;
	mo->offset = 0;
	mo->range  = length;

	mmio_normalize(mo);
	mmio_init(mo);

	return 0;
}

void mmio_cleanup(struct mmio_options *mo)
{
	if (munmap(mo->iomem, mo->iosize))
		die_errno("can't unmap @ %lX", mo->iobase);
}



static void mmio_usage(const char *msg)
{
	if (msg != NULL)
		die("%s\n%s", msg, usage);
	else
		die("%s", usage);

	exit(1);
}

static void mmio_check(struct mmio_options *mo,
		       char *addr,
		       char *data)
{
	char *ptr;
	unsigned long   iobase, range = 1;

	ptr = strchr(addr, '@');
	if (ptr != NULL) {
		*ptr = '\0';
		++ ptr;
	}

	if (parse_ulong(&iobase, addr) < 0)
		die("cannot parse '%s' as unsigned long.\n", addr);

	if (ptr && parse_ulong(&range, ptr) < 0)
		die("cannot parse '%s' as uint32_t.\n", ptr);

	if (data != NULL) {
		if (parse_uint32(&mo->value, data) < 0)
			die("cannot parse '%s' as unsigned long.\n", data);
		mo->mode = 1;
	} else {
		mo->mode = 0;
	}

	mmio_map(mo, iobase, range);
}

static void mmio_parse(struct mmio_options *mo, int argc, char *argv[])
{
	int   count;

	memset(mo, 0, sizeof(*mo));
	mo->range = 1;
	mo->flags = HEXDUMP_32BIT;

	for (;;) {
		int ch;

		ch = getopt(argc, argv, "abhxk");
		if (ch == -1)
			break;

		switch (ch) {
		case 'a':
			mo->flags = HEXDUMP_ASCII;
			break;

		case 'b':
			mo->flags = HEXDUMP_8BIT;
			break;

		case 'h':
			mo->flags = HEXDUMP_16BIT;
			break;

		case 'x':
			mo->flags = HEXDUMP_32BIT;
			break;

		case 'k':
			mo->kmem = 1;
			break;

		default:
			exit(1);
		}
	}

	count = argc - optind;
	if (count == 0)
		mmio_usage("command line arguments missing");
	if (count > 2)
		mmio_usage("too many command line arguments");

	char *p1 = argv[optind ++];
	char *p2 = argv[optind ++];

	mmio_check(mo, p1, p2);
}

static void mmio_read(struct mmio_options *mo)
{
	mmio_hexdump(mo, mo->flags);
}

static void mmio_write(struct mmio_options *mo)
{
	uint32_t data;

	writel(mo->value, mo->iomem + mo->offset);
	printf("W@ %08lX: %08X\n", mo->iobase + mo->offset, mo->value);

	data = readl(mo->iomem + mo->offset);
	if (data != mo->value)
		printf("Wrote %08X and read again %08X, r/o register ?\n",
		       mo->value, data);
}

static void mmio_run(struct mmio_options *mo)
{
	if (mo->mode == 0) {
		mmio_read(mo);
	} else if (mo->mode == 1) {
		mmio_write(mo);
	} else {
		die("Unknown MMIO mode:%d\n", mo->mode);
	}
}

int main(int argc, char *argv[])
{
	struct mmio_options mo;

	mmio_parse(&mo, argc, argv);
	mmio_run(&mo);
	mmio_cleanup(&mo);

	return 0;
}
