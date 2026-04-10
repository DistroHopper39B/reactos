/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Stub and unimplemented functions for the original Apple TV
 * COPYRIGHT:   Copyright 2023-2024 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

/* FUNCTIONS *****************************************************************/

/* GENERAL FUNCTIONS *********************************************************/

VOID
AppleTVGetExtendedBIOSData(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize)
{
    /* Does not exist */
}

VOID
AppleTVHwIdle(VOID)
{
    /* Does not exist */
}

VOID
AppleTVBeep(VOID)
{
    /* No beeper speaker support */
}

VOID
ChainLoadBiosBootSectorCode(UCHAR BootDrive, ULONG BootPartition)
{
    /* Not supported */
}

VOID
DiskStopFloppyMotor(VOID)
{
    /* Not supported */
}

VOID
DriveMapMapDrivesInSection(ULONG_PTR SectionId)
{
    /* Not supported */
}

USHORT __cdecl
PxeCallApi(USHORT Segment, USHORT Offset, USHORT Service, void *Parameter)
{
    /* Not supported */
    return 0;
}

VOID
Relocator16Boot(REGS *In, USHORT StackSegment, USHORT StackPointer, USHORT CodeSegment, USHORT CodePointer)
{
    /* Not supported */
    while (1);
}

int
__cdecl
Int386(int ivec, REGS* in, REGS* out)
{
    /* Not supported */
    return 0;
}
