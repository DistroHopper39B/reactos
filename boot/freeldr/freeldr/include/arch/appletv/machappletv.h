/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Header file for original Apple TV
 * COPYRIGHT:   Copyright 2023 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

#pragma once

#ifndef __MEMORY_H
#include "mm.h"
#endif

/* UEFI support */
#include <Uefi.h>
#include <Acpi.h>
#include <GraphicsOutput.h>

#include "boot_args.h"

VOID
AppleTVConsPutChar(int Ch);

BOOLEAN
AppleTVConsKbHit(VOID);

int
AppleTVConsGetCh(VOID);

VOID
AppleTVInitializeVideo(VOID);

VOID
AppleTVVideoClearScreen(UCHAR Attr);

VIDEODISPLAYMODE
AppleTVVideoSetDisplayMode(PCSTR DisplayMode, BOOLEAN Init);

VOID
AppleTVVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);

ULONG
AppleTVVideoGetBufferSize(VOID);

VOID
AppleTVVideoGetFontsFromFirmware(PULONG RomFontPointers);

VOID
AppleTVVideoSetTextCursorPosition(UCHAR X, UCHAR Y);

VOID
AppleTVVideoHideShowTextCursor(BOOLEAN Show);

VOID
AppleTVVideoPutChar(int Ch, UCHAR Attr,
                    unsigned X, unsigned Y);

                    
VOID
AppleTVVideoCopyOffScreenBufferToVRAM(PVOID Buffer);

BOOLEAN
AppleTVVideoIsPaletteFixed(VOID);

VOID
AppleTVVideoSetPaletteColor(UCHAR Color, UCHAR Red,
                            UCHAR Green, UCHAR Blue);

VOID
AppleTVVideoGetPaletteColor(UCHAR Color, UCHAR* Red,
                            UCHAR* Green, UCHAR* Blue);

VOID
AppleTVVideoSync(VOID);


VOID
AppleTVBeep(VOID);

PFREELDR_MEMORY_DESCRIPTOR
AppleTVMemGetMemoryMap(ULONG *MemoryMapSize);

VOID AppleTVGetExtendedBIOSData(PULONG ExtendedBIOSDataArea,
                                PULONG ExtendedBIOSDataSize);


VOID
AppleTVMemInit(VOID);

UCHAR
AppleTVGetFloppyCount(VOID);

BOOLEAN
AppleTVDiskReadLogicalSectors(IN UCHAR DriveNumber,
                              IN ULONGLONG SectorNumber,
                              IN ULONG SectorCount,
                              OUT PVOID Buffer);

BOOLEAN AppleTVDiskGetDriveGeometry(UCHAR DriveNumber,
                                    PGEOMETRY DriveGeometry);

ULONG
AppleTVDiskGetCacheableBlockCount(UCHAR DriveNumber);

TIMEINFO*
AppleTVGetTime(VOID);

BOOLEAN
PcInitializeBootDevices(VOID);

PCONFIGURATION_COMPONENT_DATA
AppleTVHwDetect(
    _In_opt_ PCSTR Options);

VOID
AppleTVHwIdle(VOID);

VOID
AppleTVPrepareForReactOS(VOID);

VOID
AppleTVDiskInit();

CONFIGURATION_TYPE
DiskGetConfigType(
    _In_ UCHAR DriveNumber);

/* Platform-specific boot drive and partition numbers */
extern UCHAR FrldrBootDrive;
extern ULONG FrldrBootPartition;

/* Mach-O boot args pointer */
extern PMACH_BOOTARGS BootArgs;

/* EFI system table */
extern EFI_SYSTEM_TABLE *GlobalSystemTable;