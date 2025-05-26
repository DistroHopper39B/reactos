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

extern PMACH_BOOTARGS BootArgs; // from eax register; see appletventry.S

UCHAR FrldrBootDrive;
ULONG FrldrBootPartition;

#define SMBIOS_TABLE_GUID \
  { \
    0xeb9d2d31, 0x2d88, 0x11d3, {0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define SMBIOS_TABLE_LOW 0xF0000

/* FUNCTIONS *****************************************************************/

VOID
CopySmbios(VOID)
{
    PSMBIOS_TABLE_HEADER    SmbiosTable = NULL;
    EFI_SYSTEM_TABLE        *EfiSystemTable;
    EFI_GUID                SmbiosTableGuid = SMBIOS_TABLE_GUID;
    UINTN                   i;
    
    EfiSystemTable = (EFI_SYSTEM_TABLE *) BootArgs->EfiSystemTable;
    
    for (i = 0; i < EfiSystemTable->NumberOfTableEntries; i++)
    {
        if (!memcmp(&EfiSystemTable->ConfigurationTable[i].VendorGuid,
                    &SmbiosTableGuid, sizeof(SmbiosTableGuid)))
        {
            SmbiosTable = (PSMBIOS_TABLE_HEADER) EfiSystemTable->ConfigurationTable[i]
                                                    .VendorTable;
            break;
        }
    }
    
    if (!SmbiosTable)
    {
        ERR("Cannot find SMBIOS!\n");
    }
    
    memcpy((PVOID) SMBIOS_TABLE_LOW, SmbiosTable, sizeof(SMBIOS_TABLE_HEADER));
}

VOID
AppleTVPrepareForReactOS(VOID)
{
    CopySmbios();

    DebugDisableScreenPort();
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
    
    // FIXME: Do this somewhere else!
    FrldrBootDrive = 0x80;
    FrldrBootPartition = 1;
    
    // If verbose mode is enabled according to Mach, enable it here
    if (BootArgs->Video.DisplayMode == DISPLAY_MODE_TEXT)
    {
        // Clear screen
        AppleTVVideoClearScreen(COLOR_BLACK);
        
        // Enable screen debug
        DebugEnableScreenPort();
    }
    
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