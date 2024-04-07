/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Stub and unimplemented functions for the original Apple TV
 * COPYRIGHT:   Copyright 2023 DistroHopper39B (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

/* FUNCTIONS *****************************************************************/

/* GENERAL FUNCTIONS *********************************************************/

UCHAR
AppleTVGetFloppyCount(VOID)
{
    /* No floppy drive present */
    return 0;
}

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

UCHAR
DriveMapGetBiosDriveNumber(PCSTR DeviceName)
{
    return 0;
}

int __cdecl
Int386(int ivec, REGS* in, REGS* out)
{
    return 0;
}

BOOLEAN
PxeInit(VOID)
{
    return FALSE;
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

VIDEODISPLAYMODE
AppleTVVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init)
{
    // We only have one display mode
    return VideoTextMode;
}

BOOLEAN
AppleTVVideoIsPaletteFixed(VOID)
{
    return FALSE;
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