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

/* Video parameters passed to kernel. */
typedef struct {
    UINT32 BaseAddress; /* Base video address */
    UINT32 DisplayMode; /* Display mode specifier */
    UINT32 Pitch; /* Bytes per row */
    UINT32 Width; /* Display width in pixels */
    UINT32 Height; /* Display height in pixels */
    UINT32 Depth; /* Display depth in bits */
} __attribute__((aligned(4))) MACH_VIDEO, PMACH_VIDEO;

/* Boot arguments struct passed into loader. A pointer to this struct is located in the EAX register upon kernel load.
 * See xnu-1228 pexpert/pexpert/i386/boot.h.
 */

typedef struct {
    UINT16 Revision; /* Revision of this structure */
    UINT16 Version; /* Version of this structure */

    char CmdLine[MACH_CMDLINE]; /* Command line data */

    UINT32 EfiMemoryMap; /* Location of EFI memory map */
    UINT32 EfiMemoryMapSize; /* Size of EFI memory map */
    UINT32 EfiMemoryDescriptorSize; /* Size of EFI descriptor */
    UINT32 EfiMemoryDescriptorVersion; /* Version of EFI memory descriptors */

    MACH_VIDEO Video; /* Video parameters */

    UINT32 DeviceTree; /* Pointer to base of Apple IODeviceTree */
    UINT32 DeviceTreeLength; /* Length of device tree */

    UINT32 KernelBaseAddress; /* Beginning of kernel as specified by `-segaddr __TEXT` */
    UINT32 KernelSize; /* Size of kernel and firmware */

    UINT32 EfiRuntimeServicesPageStart; /* Address of defragmented runtime pages */
    UINT32 EfiRuntimeServicesPageCount; /* Number of EFI pages */
    UINT32 EfiSystemTable; /* EFI System Table */

    UINT8 EfiMode; /* EFI mode: 32 = 32 bit EFI, 64 = 64 bit EFI */

    UINT8 __reserved1[3];
    UINT8 __reserved2[7];
} __attribute__((aligned(4))) MACH_BOOTARGS, *PMACH_BOOTARGS;

extern PMACH_BOOTARGS BootArgs;
