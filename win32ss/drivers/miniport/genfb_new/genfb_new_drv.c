/*
 * PROJECT:     New Generic Framebuffer Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main file.
 * COPYRIGHT:   Copyright 2024 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

#include <ntifs.h>
#include <arc/arc.h>

#include <dderror.h>
#define __BROKEN__
#include <miniport.h>
#include <video.h>
#include <devioctl.h>

#include <section_attribs.h>

#include <debug.h>
// #define DPRINT(fmt, ...)    VideoDebugPrint((Info, fmt, ##__VA_ARGS__))
// #define DPRINT1(fmt, ...)   VideoDebugPrint((Error, fmt, ##__VA_ARGS__))

#include <drivers/bootvid/framebuf.h>
#include <drivers/bootvid/framebuf.c> // FIXME: Temporary HACK

