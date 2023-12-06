/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Real-time clock access routine for the original Apple TV
 * COPYRIGHT:   Copyright 2023 DistroHopper39B (distrohopper39b.business@gmail.com)
 */

#include <freeldr.h>
#include <Uefi.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

EFI_SYSTEM_TABLE *EfiSystemTable;

TIMEINFO *
AppleTVGetTime(VOID)
{
    static TIMEINFO TimeInfo;
    EFI_STATUS Status;
    EFI_TIME time = {0};

    EfiSystemTable = (EFI_SYSTEM_TABLE *) appletv_boot_info->efi_system_table_ptr;

    Status = EfiSystemTable->RuntimeServices->GetTime(&time, NULL);
    if (Status != EFI_SUCCESS)
        ERR("cannot get time status %d\n", Status);

    TimeInfo.Year = time.Year;
    TimeInfo.Month = time.Month;
    TimeInfo.Day = time.Day;
    TimeInfo.Hour = time.Hour;
    TimeInfo.Minute = time.Minute;
    TimeInfo.Second = time.Second;
    return &TimeInfo;
}