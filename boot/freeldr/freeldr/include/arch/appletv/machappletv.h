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

#include "boot_args.h"

VOID AppleTVConsPutChar(int Ch);

BOOLEAN AppleTVConsKbHit(VOID);
int AppleTVConsGetCh(VOID);

VOID AppleTVVideoClearScreen(UCHAR Attr);
VIDEODISPLAYMODE AppleTVVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init);
VOID AppleTVVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);
ULONG AppleTVVideoGetBufferSize(VOID);
VOID AppleTVVideoGetFontsFromFirmware(PULONG RomFontPointers);
VOID AppleTVVideoSetTextCursorPosition(UCHAR X, UCHAR Y);
VOID AppleTVVideoHideShowTextCursor(BOOLEAN Show);
VOID AppleTVVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y);
VOID AppleTVVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOLEAN AppleTVVideoIsPaletteFixed(VOID);
VOID AppleTVVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID AppleTVVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
VOID AppleTVVideoSync(VOID);
VOID AppleTVVideoScrollUp(VOID);
VOID AppleTVPrepareForReactOS(VOID);

VOID AppleTVMemInit(VOID);
PFREELDR_MEMORY_DESCRIPTOR AppleTVMemGetMemoryMap(ULONG *MemoryMapSize);

BOOLEAN AppleTVInitializeBootDevices(VOID);
VOID AppleTVDiskInit();
BOOLEAN AppleTVDiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOLEAN AppleTVDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY DriveGeometry);
ULONG AppleTVDiskGetCacheableBlockCount(UCHAR DriveNumber);
BOOLEAN PcInitializeBootDevices(VOID);

TIMEINFO* AppleTVGetTime(VOID);

PCONFIGURATION_COMPONENT_DATA AppleTVHwDetect(_In_opt_ PCSTR Options);
VOID AppleTVHwIdle(VOID);

VOID AppleTVBeep(VOID);

/* appletvstubs.c */
UCHAR AppleTVGetFloppyCount(VOID);
VOID AppleTVGetExtendedBIOSData(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize);
VOID AppleTVHwIdle(VOID);
VOID AppleTVBeep(VOID);

/* Platform-specific boot drive and partition numbers */
extern UCHAR FrldrBootDrive;
extern ULONG FrldrBootPartition;
LONG DiskReportError(BOOLEAN bShowError);
