/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Stub and unimplemented functions for the original Apple TV
 * COPYRIGHT:   Copyright 2023 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

/* FUNCTIONS *****************************************************************/

/* GENERAL FUNCTIONS *********************************************************/

VOID
AppleTVGetExtendedBIOSData(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize)
{
    /* No EBDA on EFI-based Apple TV */
}

VOID
AppleTVHwIdle(VOID)
{
    /* No traditional HwIdle on Apple TV */
}

VOID
AppleTVBeep(VOID)
{
    /* No beeper speaker support */
}

VOID
ChainLoadBiosBootSectorCode(UCHAR BootDrive, ULONG BootPartition)
{
    // Not supported
}

VOID
DiskStopFloppyMotor(VOID)
{
    // Not supported
}

VOID
DriveMapMapDrivesInSection(ULONG_PTR SectionId)
{
    // Not supported
}

USHORT __cdecl
PxeCallApi(USHORT Segment, USHORT Offset, USHORT Service, void *Parameter)
{
    // Not supported
    return 0;
}

VOID
Relocator16Boot(REGS *In, USHORT StackSegment, USHORT StackPointer, USHORT CodeSegment, USHORT CodePointer)
{
    // Not supported
    while (1);
}

/* VIDEO FUNCTIONS ***********************************************************/

VOID
AppleTVVideoGetFontsFromFirmware(PULONG RomFontPointers)
{
    // Might technically be possible, but we're using a custom bitmap font.
}

VOID
AppleTVVideoSetTextCursorPosition(UCHAR X, UCHAR Y)
{
    // We don't have a cursor
}

VOID
AppleTVVideoHideShowTextCursor(BOOLEAN Show)
{
    // We don't have a cursor
}

VOID
AppleTVVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
{
    // Not supported
}

VOID
AppleTVVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue)
{
    // Not supported
}

VOID
AppleTVVideoSync(VOID)
{
    // Not supported
}