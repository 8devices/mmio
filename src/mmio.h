/*
 * Copyright (C) 2012 Žilvinas Valinskas
 * See LICENSE for more information.
 */
#ifndef __MMIO_H__
#define __MMIO_H__

#include <sys/types.h>
#include <stdint.h>

struct mmio_options
{
	unsigned long 	iobase;		/* getpagesize() aligned, see mmap(2) */
	unsigned long	offset;		/* additional offset from iobase */
	unsigned long	range;		/* N * uint32_t read/write ops. */
	uint32_t	value;		/* 32 bit values only, for now. */
	uint32_t	flags;

	int		verbose;
	int		ascii;		/* print ASCII */
	int		forced;
	int		mode;		/* 0 - read, 1 - write */
	int		kmem;		/* 0 - /dev/mem, 1 - /dev/kmem */

	void		*iomem;
	size_t		iosize;
};

int  mmio_map(struct mmio_options *mo, unsigned long base, size_t length);
void mmio_init(struct mmio_options *mo);
void mmio_cleanup(struct mmio_options *mo);
void mmio_hexdump(const struct mmio_options *mo, size_t flags);

uint32_t mmio_readl(const struct mmio_options *mo, unsigned int offset);
void mmio_writel(const struct mmio_options *mo, unsigned int offset, uint32_t value);

static inline uint32_t readl(void *ptr)
{
	uint32_t *data = ptr;
	return *data;
}

static inline void writel(uint32_t value, void *ptr)
{
	uint32_t *data = ptr;
	*data = value;
}


#endif //__MMIO_H__
