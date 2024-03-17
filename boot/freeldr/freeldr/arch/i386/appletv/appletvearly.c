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

/* FUNCTIONS *****************************************************************/

VOID
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
    
    // Start BootMain
    BootMain(BootArgs->CmdLine);
}

VOID
Reboot(VOID)
{
    if (BootArgs)
    {
        // Do UEFI reboot
        ((EFI_SYSTEM_TABLE *) BootArgs->EfiSystemTable)
                            ->RuntimeServices->ResetSystem(EfiResetCold,
                            EFI_SUCCESS, 0, NULL);
        // if it fails, hang
        while (1);
    }
    else
    {
        // No UEFI reboot; hang
        while (1);
    }
}