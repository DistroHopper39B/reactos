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

ULONG_PTR VramAddress;
ULONG VramSize;
PCM_FRAMEBUF_DEVICE_DATA FrameBufferData = NULL;

extern UCHAR BitmapFont8x16[256 * 16];

UCHAR MachDefaultTextColor = COLOR_GRAY;

static PIXEL_BITMASK EfiPixelMasks[] =
{ /* Red,        Green,      Blue,       Reserved */
    {0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000},   // PixelRedGreenBlueReserved8BitPerColor
    {0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000},   // PixelBlueGreenRedReserved8BitPerColor
    {0,          0,          0,          0}             // PixelBitMask, PixelBltOnly, ...
};

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
    PIXEL_BITMASK AppleTVBitMask = EfiPixelMasks[PixelBlueGreenRedReserved8BitPerColor];
    
    
    
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