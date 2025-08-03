/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware-specific routines for the original Apple TV
 * COPYRIGHT:   Copyright 2023 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>
#include <Uefi.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

/* GLOBALS *******************************************************************/

UCHAR FrldrBootDrive = 0x80; // Drive 1
ULONG FrldrBootPartition = 1; // Partition 1

extern PMACH_BOOTARGS BootArgs; // from eax register; see appletventry.S

/* FUNCTIONS *****************************************************************/

VOID
AppleTVPrepareForReactOS(VOID)
{
    DebugDisableScreenPort();
}

static
VOID
ParseCmdLine(PCSTR CmdLine)
{
    PSTR    CmdLineCopy = NULL, PartitionString = NULL;
    CHAR    CmdLineCopyBuffer[1024];
    ULONG   Value;
    
    // Don't bother checking the command line if it's empty
    if (!CmdLine || !*CmdLine) return;
    
    strncpy(CmdLineCopyBuffer,
        CmdLine,
        sizeof(CmdLineCopyBuffer));
    
    CmdLineCopy = CmdLineCopyBuffer;
    
    // Uppercase
    _strupr(CmdLineCopy);
    
    // Get the partition number
    PartitionString = strstr(CmdLineCopy, "PARTITION");
    
    /*
     * Check if we got a partition number.
     * NOTE: Inspired by freeldr/lib/debug.c, DebugInit(),
     * which is inspired by reactos/ntoskrnl/kd/kdinit.c, KdInitSystem(...)
     */
    if (PartitionString)
    {
        // Move past the actual string, to reach the partition number
        PartitionString += strlen("PARTITION");
        
        // Now get past any spaces
        while (*PartitionString == ' ') PartitionString++;
        
        // And make sure we have a partition number
        if (*PartitionString)
        {
            Value = atol(PartitionString + 1);
            if (Value) FrldrBootPartition = Value;
        }
    }
}

VOID
MachInit(const char *CmdLine)
{
    if (BootArgs->Version != 1
        && BootArgs->Revision != 4)
    {
        ERR("This is not an Apple TV!\n");
        
        _disable();
        __halt();
        for (;;);
    }
        
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
    
    // If verbose mode is enabled according to Mach, enable it here
    if (BootArgs->Video.DisplayMode == DISPLAY_MODE_TEXT)
    {
        // Clear screen
        AppleTVVideoClearScreen(COLOR_BLACK);
        
        // Enable screen debug
        DebugEnableScreenPort();
    }
    
    // If a disk partition is specified on the command line, set it.
    ParseCmdLine(CmdLine);
    
    HalpCalibrateStallExecution();
}

VOID
__cdecl
Reboot(VOID)
{
    if (BootArgs)
    {
        // Do UEFI reboot
        EFI_RESET_SYSTEM ResetSystem = ((EFI_SYSTEM_TABLE *) BootArgs->EfiSystemTable)->RuntimeServices->ResetSystem;
        ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
        // if it fails, hang
        _disable();
        __halt();
        for (;;);
    }
    else
    {
        // No UEFI reboot; hang
        _disable();
        __halt();
        for (;;);
    }
}