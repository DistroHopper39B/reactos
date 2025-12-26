/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video support for the original Apple TV
 * COPYRIGHT:   Copyright 2023 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/* INCLUDES *******************************************************************/
#include <freeldr.h>
#include "../../vidfb.h"

/* UEFI support */
#include <Uefi.h>
#include <GraphicsOutput.h>

/* GLOBALS ********************************************************************/

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16
#define TOP_BOTTOM_LINES 0

extern UCHAR BitmapFont8x16[256 * 16];

UCHAR MachDefaultTextColor = COLOR_GRAY;

/* FUNCTIONS ******************************************************************/

VOID
AppleTVVideoClearScreen(UCHAR Attr)
{
    VidFbClearScreen(Attr);
}

VOID
AppleTVVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
    VidFbPutChar(Ch, Attr, X, Y);
}

VOID
AppleTVVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    VidFbGetDisplaySize(Width, Height, Depth);
}

ULONG
AppleTVVideoGetBufferSize(VOID)
{
    return VidFbGetBufferSize();
}

VOID
AppleTVVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
    VidFbCopyOffScreenBufferToVRAM(Buffer);
}

VOID
AppleTVInitializeVideo(VOID)
{
    PMACH_VIDEO Video = &BootArgs->Video;
    
    VidFbInitializeVideo(Video->BaseAddress,
                            (Video->Pitch * Video->Height),
                            Video->Width,
                            Video->Height,
                            (Video->Pitch / 4),
                            Video->Depth);
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