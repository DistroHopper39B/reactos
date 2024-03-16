/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware-specific routines for the original Apple TV
 * COPYRIGHT:   Copyright 2023 DistroHopper39B (distrohopper39b.business@gmail.com)
 */

#include <freeldr.h>
#include <drivers/bootvid/framebuf.h>
#include <Uefi.h>
#include <GraphicsOutput.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

VOID
AppleTVPrepareForReactOS(VOID)
{
    AppleTVDiskInit(FALSE);

    DebugDisableScreenPort();
}

VOID
MachInit(const char *CmdLine)
{
    
    ERR("Boot args: 0x%X\n", BootArgs);
    /* Setup vtbl */
    RtlZeroMemory(&MachVtbl, sizeof(MachVtbl));
    MachVtbl.ConsPutChar = AppleTVConsPutChar;
    MachVtbl.ConsKbHit = AppleTVConsKbHit;
    MachVtbl.ConsGetCh = AppleTVConsGetCh;
    MachVtbl.VideoClearScreen = AppleTVVideoClearScreen;
    MachVtbl.VideoSetDisplayMode = AppleTVVideoSetDisplayMode;
    MachVtbl.VideoGetDisplaySize = AppleTVVideoGetDisplaySize;
    MachVtbl.VideoGetBufferSize = AppleTVVideoGetBufferSize;
    MachVtbl.VideoGetFontsFromFirmware = AppleTVVideoGetFontsFromFirmware;
    MachVtbl.VideoSetTextCursorPosition = AppleTVVideoSetTextCursorPosition;
    MachVtbl.VideoHideShowTextCursor = AppleTVVideoHideShowTextCursor;
    MachVtbl.VideoPutChar = AppleTVVideoPutChar;
    MachVtbl.VideoCopyOffScreenBufferToVRAM = AppleTVVideoCopyOffScreenBufferToVRAM;
    MachVtbl.VideoIsPaletteFixed = AppleTVVideoIsPaletteFixed;
    MachVtbl.VideoSetPaletteColor = AppleTVVideoSetPaletteColor;
    MachVtbl.VideoGetPaletteColor = AppleTVVideoGetPaletteColor;
    MachVtbl.VideoSync = AppleTVVideoSync;
    MachVtbl.Beep = AppleTVBeep;
    MachVtbl.PrepareForReactOS = AppleTVPrepareForReactOS;
    MachVtbl.GetMemoryMap = AppleTVMemGetMemoryMap;
    MachVtbl.GetExtendedBIOSData = AppleTVGetExtendedBIOSData;
    MachVtbl.GetFloppyCount = AppleTVGetFloppyCount;
    MachVtbl.DiskReadLogicalSectors = AppleTVDiskReadLogicalSectors;
    MachVtbl.DiskGetDriveGeometry = AppleTVDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = AppleTVDiskGetCacheableBlockCount;
    MachVtbl.GetTime = AppleTVGetTime;
    MachVtbl.InitializeBootDevices = PcInitializeBootDevices; // in hwdisk.c
    MachVtbl.HwDetect = AppleTVHwDetect;
    MachVtbl.HwIdle = AppleTVHwIdle;

    while (1);
    
    HalpCalibrateStallExecution();
}