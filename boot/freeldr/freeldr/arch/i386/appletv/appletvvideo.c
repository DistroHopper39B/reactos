/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video support for the original Apple TV
 * COPYRIGHT:   Copyright 2023 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/* INCLUDES *******************************************************************/
#include <freeldr.h>

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


static ULONG
AppleTVVideoAttrToSingleColor(UCHAR Attr)
{
    UCHAR Intensity;
    Intensity = (0 == (Attr & 0x08) ? 127 : 255);

    return 0xff000000 |
           (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
           (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
           (0 == (Attr & 0x01) ? 0 : Intensity);
}

static VOID
AppleTVVideoAttrToColors(UCHAR Attr, ULONG *FgColor, ULONG *BgColor)
{
    *FgColor = AppleTVVideoAttrToSingleColor(Attr & 0xf);
    *BgColor = AppleTVVideoAttrToSingleColor((Attr >> 4) & 0xf);
}


static VOID
AppleTVVideoClearScreenColor(ULONG Color, BOOLEAN FullScreen)
{
    ULONG Delta;
    ULONG Line, Col;
    PULONG p;
    
    PMACH_VIDEO Video = &BootArgs->Video;

    Delta = (Video->Pitch + 3) & ~ 0x3;
    for (Line = 0; Line < Video->Height - (FullScreen ? 0 : 2 * TOP_BOTTOM_LINES); Line++)
    {
        p = (PULONG) ((char *) Video->BaseAddress + (Line + (FullScreen ? 0 : TOP_BOTTOM_LINES)) * Delta);
        for (Col = 0; Col < Video->Width; Col++)
        {
            *p++ = Color;
        }
    }
}

VOID
AppleTVVideoClearScreen(UCHAR Attr)
{
    ULONG FgColor, BgColor;

    AppleTVVideoAttrToColors(Attr, &FgColor, &BgColor);
    AppleTVVideoClearScreenColor(BgColor, FALSE);
}

VOID
AppleTVVideoOutputChar(UCHAR Char, unsigned X, unsigned Y, ULONG FgColor, ULONG BgColor)
{
    PUCHAR FontPtr;
    PULONG Pixel;
    UCHAR Mask;
    unsigned Line;
    unsigned Col;
    ULONG Delta;
    
    PMACH_VIDEO Video = &BootArgs->Video;
    
    Delta = (Video->Pitch + 3) & ~ 0x3;
    FontPtr = BitmapFont8x16 + Char * 16;
    Pixel = (PULONG) ((char *) Video->BaseAddress +
            (Y * CHAR_HEIGHT + TOP_BOTTOM_LINES) *  Delta + X * CHAR_WIDTH * 4);

    for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
        Mask = 0x80;
        for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
            Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? FgColor : BgColor);
            Mask = Mask >> 1;
        }
        Pixel = (PULONG) ((char *) Pixel + Delta);
    }
}

VOID
AppleTVVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
    ULONG FgColor = 0;
    ULONG BgColor = 0;
    if (Ch != 0)
    {
        AppleTVVideoAttrToColors(Attr, &FgColor, &BgColor);
        AppleTVVideoOutputChar(Ch, X, Y, FgColor, BgColor);
    }
}

VOID
AppleTVVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    PMACH_VIDEO Video = &BootArgs->Video;
    
    *Width =  Video->Width / CHAR_WIDTH;
    *Height = (Video->Height - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT;
    *Depth =  0;
}

ULONG
AppleTVVideoGetBufferSize(VOID)
{
    PMACH_VIDEO Video = &BootArgs->Video;
    
    return ((Video->Height - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT * (Video->Width / CHAR_WIDTH) * 2);
}

VOID
AppleTVVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
    PMACH_VIDEO Video = &BootArgs->Video;
    PUCHAR OffScreenBuffer = (PUCHAR)Buffer;

    ULONG Col, Line;
    for (Line = 0; Line < (Video->Height - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT; Line++)
    {
        for (Col = 0; Col < Video->Width / CHAR_WIDTH; Col++)
        {
            AppleTVVideoPutChar(OffScreenBuffer[0], OffScreenBuffer[1], Col, Line);
            OffScreenBuffer += 2;
        }
    }
}

VOID
AppleTVVideoScrollUp(VOID)
{
    PMACH_VIDEO Video = &BootArgs->Video;
    
    ULONG BgColor, Dummy;
    ULONG Delta;
    Delta = (Video->Pitch + 3) & ~ 0x3;
    ULONG PixelCount = Video->Width * CHAR_HEIGHT *
                       (((Video->Height - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT) - 1);
    PULONG Src = (PULONG)((PUCHAR)Video->BaseAddress + (CHAR_HEIGHT + TOP_BOTTOM_LINES) * Delta);
    PULONG Dst = (PULONG)((PUCHAR)Video->BaseAddress + TOP_BOTTOM_LINES * Delta);

    AppleTVVideoAttrToColors(ATTR(COLOR_WHITE, COLOR_BLACK), &Dummy, &BgColor);

    while (PixelCount--)
        *Dst++ = *Src++;

    for (PixelCount = 0; PixelCount < Video->Width * CHAR_HEIGHT; PixelCount++)
        *Dst++ = BgColor;
}