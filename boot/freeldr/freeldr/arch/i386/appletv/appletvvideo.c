/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video support for the original Apple TV
 * COPYRIGHT:   Authors of uefivid.c
 *              Copyright 2023-2026 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/* INCLUDES *******************************************************************/
#include <freeldr.h>
#include "../../vidfb.h"

/* GLOBALS ********************************************************************/

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16
#define TOP_BOTTOM_LINES 0

ULONG_PTR VramAddress;
ULONG VramSize;
PCM_FRAMEBUF_DEVICE_DATA FrameBufferData = NULL;

extern UCHAR BitmapFont8x16[256 * 16];

UCHAR MachDefaultTextColor = COLOR_GRAY;

/* FUNCTIONS ******************************************************************/

VOID
AppleTVVideoClearScreen(UCHAR Attr)
{
    FbConsClearScreen(Attr);
}

VOID
AppleTVVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
    FbConsPutChar(Ch, Attr, X, Y);
}

VOID
AppleTVVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    FbConsGetDisplaySize(Width, Height, Depth);
}

ULONG
AppleTVVideoGetBufferSize(VOID)
{
    return FbConsGetBufferSize();
}

VOID
AppleTVVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
    FbConsCopyOffScreenBufferToVRAM(Buffer);
}

VOID
AppleTVInitializeVideo(VOID)
{
    PMACH_VIDEO Video = &BootArgs->Video;

    VramAddress = Video->BaseAddress;
    VramSize = (Video->Pitch * Video->Height);

    /* PixelBlueGreenRedReserved8BitPerColor */
    PIXEL_BITMASK AppleTVBitMask = {0x00FF0000,
                                    0x0000FF00,
                                    0x000000FF,
                                    0xFF000000};

    VidFbInitializeVideo(&FrameBufferData,
                         VramAddress,
                         VramSize,
                         Video->Width,
                         Video->Height,
                         (Video->Pitch / 4),
                         Video->Depth,
                         &AppleTVBitMask);
}

VIDEODISPLAYMODE
AppleTVVideoSetDisplayMode(PCSTR DisplayMode, BOOLEAN Init)
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
AppleTVVideoGetFontsFromFirmware(PULONG RomFontPointers)
{
    /* Not supported */
}

VOID
AppleTVVideoSetTextCursorPosition(UCHAR X, UCHAR Y)
{
    /* We don't have a cursor */
}

VOID
AppleTVVideoHideShowTextCursor(BOOLEAN Show)
{
    /* We don't have a cursor */
}

VOID
AppleTVVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
{
    /* Not supported */
}

VOID
AppleTVVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue)
{
    /* Not supported */
}

VOID
AppleTVVideoSync(VOID)
{
    /* Not supported */
}
