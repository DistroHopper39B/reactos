/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Apple TV boot info
 * COPYRIGHT:   Copyright 2023 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

#pragma once

#define MACH_CMDLINE 1024

#define DISPLAY_MODE_GRAPHICS 1
#define DISPLAY_MODE_TEXT 2

#ifdef _MSC_VER
#define ALIGNED(a) __declspec(align(a))
#else
#define ALIGNED(a) __attribute__((aligned(a)))
#endif

typedef struct ALIGNED(4)
{
    UINT32      BaseAddress;
    UINT32      DisplayMode;
    UINT32      Pitch;
    UINT32      Width;
    UINT32      Height;
    UINT32      Depth;
} MACH_VIDEO, *PMACH_VIDEO;

/*
 * Boot arguments struct passed into loader. A pointer to this struct is located in the EAX register upon kernel load.
 * See xnu-1228 pexpert/pexpert/i386/boot.h.
 */

typedef struct ALIGNED(4)
{
    UINT16      Revision;
    UINT16      Version;

    CHAR        CmdLine[MACH_CMDLINE];

    UINT32      EfiMemoryMap;
    UINT32      EfiMemoryMapSize;
    UINT32      EfiMemoryDescriptorSize;
    UINT32      EfiMemoryDescriptorVersion;

    MACH_VIDEO  Video;

    UINT32      DeviceTree;
    UINT32      DeviceTreeLength;

    UINT32      KernelBaseAddress;
    UINT32      KernelSize;

    UINT32      EfiRuntimeServicesPageStart;
    UINT32      EfiRuntimeServicesPageCount;
    UINT32      EfiSystemTable;

    UINT8       EfiMode;
    UINT8       __reserved1[3];
    UINT32      __reserved2[7];
} MACH_BOOTARGS, *PMACH_BOOTARGS;
