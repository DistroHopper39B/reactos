/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Apple TV preboot environment
 * COPYRIGHT:   Copyright 2023 DistroHopper39B (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>
#include <Uefi.h>

/* GLOBALS *******************************************************************/

PMACH_BOOTARGS BootArgs;
extern UINT32 BootArgPtr;
extern ULONG DebugPort;

#define SCREEN 1

/* FUNCTIONS *****************************************************************/

static
VOID
AppleTVSetupCmdLine(IN PCCH CmdLine)
{
    // If verbose mode is enabled according to Mach, enable it here
    if (strstr(CmdLine, "-v\0") || strstr(CmdLine, "-v ") || // Command-V (verbose mode)
        strstr(CmdLine, "-s\0") || strstr(CmdLine, "-s "))   // Command-S (single-user mode)
    {
        // Clear screen
        AppleTVVideoClearScreen(0x00);
        
        // Enable screen debug
        DebugPort |= SCREEN;
    }
}

VOID
__cdecl
AppleTVEarlyInit(VOID)
{
    // set up boot args
    BootArgs = (PMACH_BOOTARGS) BootArgPtr;
    if (!BootArgs)
    {
        Reboot();
    }
    // Hardcode boot device to first partition of first device
    FrldrBootDrive = 0x80;
    FrldrBootPartition = 1;
    
    // Set up video
    AppleTVVideoInit();
    
    // Set up command line
    AppleTVSetupCmdLine(BootArgs->CmdLine);
    
    // Start main FreeLoader runtime
    BootMain(BootArgs->CmdLine);
}

VOID
__cdecl
Reboot(VOID)
{
    if (BootArgs)
    {
        // Do UEFI reboot
        ((EFI_SYSTEM_TABLE *) BootArgs->EfiSystemTable)
                            ->RuntimeServices->ResetSystem(EfiResetCold,
                            EFI_SUCCESS, 0, NULL);
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

