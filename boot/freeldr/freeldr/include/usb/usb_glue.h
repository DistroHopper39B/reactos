/*
 * PROJECT:     FreeLoader USB Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Glue to connect libpayload to FreeLoader
 * COPYRIGHT:   Copyright 2025 Sylas Hollander <distrohopper39b.business@gmail.com>
 */
 
#pragma once

// Of course, include FreeLoader's headers.
#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

// We also need some UNIX-style types.
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define __packed __attribute__((packed))

/* io.h */
#define readb(_a) (*(volatile const unsigned char *) (_a))
#define readw(_a) (*(volatile const unsigned short *) (_a))
#define readl(_a) (*(volatile const unsigned int *) (_a))

#define writeb(_v, _a) (*(volatile unsigned char *) (_a) = (_v))
#define writew(_v, _a) (*(volatile unsigned short *) (_a) = (_v))
#define writel(_v, _a) (*(volatile unsigned int *) (_a) = (_v))

static inline __attribute__((always_inline)) uint8_t read8(const volatile void *addr)
{
	return *((volatile uint8_t *)(addr));
}

static inline __attribute__((always_inline)) uint16_t read16(const volatile void *addr)
{
	return *((volatile uint16_t *)(addr));
}

static inline __attribute__((always_inline)) uint32_t read32(const volatile void *addr)
{
	return *((volatile uint32_t *)(addr));
}

#define inb __inbyte
#define inw __inword
#define inl __indword

#define outb __outbyte
#define outw __outword
#define outl __outdword




#define udelay(us) StallExecutionProcessor((ULONG)us)

/* delay.h */
#define NSECS_PER_SEC 1000000000
#define USECS_PER_SEC 1000000
#define MSECS_PER_SEC 1000
#define NSECS_PER_MSEC (NSECS_PER_SEC / MSECS_PER_SEC)
#define NSECS_PER_USEC (NSECS_PER_SEC / USECS_PER_SEC)
#define USECS_PER_MSEC (USECS_PER_SEC / MSECS_PER_SEC)

/**
 * Delay for a specified number of milliseconds.
 *
 * @param ms Number of milliseconds to delay for.
 */
static inline void mdelay(unsigned int ms)
{
	udelay((uint64_t)ms * NSECS_PER_USEC);
}

/**
 * Delay for a specified number of seconds.
 *
 * @param s Number of seconds to delay for.
 */
static inline void delay(unsigned int s)
{
	udelay((uint64_t)s * USECS_PER_SEC);
}

#define CONFIG(x) TRUE

// we only have physical memory
#define phys_to_virt(x) (void *) (volatile u32) (x)
#define virt_to_phys(x) (volatile u32) (x)
#define bus_to_virt phys_to_virt
#define virt_to_bus virt_to_phys

#define fatal UiMessageBoxCritical

#define TAG_USB 'DBSU'
#define TAG_USB_DMA 'MADD'

#define malloc(size) FrLdrTempAlloc(size, TAG_USB)
#define free(ptr) FrLdrTempFree(ptr, TAG_USB)

#define memalign(align, size) FrLdrTempAlloc(ALIGN_UP_BY(size, align), TAG_USB)

#define dma_initialized 1
#define dma_coherent(ptr) 1
#define dma_malloc(size) FrLdrTempAlloc(size, TAG_USB_DMA)
#define dma_memalign(align, size) FrLdrTempAlloc(ALIGN_UP_BY(size, align), TAG_USB_DMA)

static inline void *xmalloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr)
    {
        ERR("Failed to allocate %d bytes of memory\n", size);
        UiMessageBoxCritical("Failed to allocate memory");
    }
    
    return ptr;
}

static inline void *calloc(size_t num, size_t size)
{
    void *ptr = malloc(size * num);
    RtlZeroMemory(ptr, size);
    return ptr;
}

#define zalloc(size) calloc(1, size)

static inline void *xzalloc(size_t size)
{
    void *ptr = xmalloc(size);
    RtlZeroMemory(ptr, size);
    return ptr;
}
