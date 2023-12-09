/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video support for the original Apple TV
 * COPYRIGHT:   Copyright 2023 DistroHopper39B (distrohopper39b.business@gmail.com)
 *              See ../xbox/xboxvideo.c
 */

// Note: This code is heavily based on xboxvideo.c, as the Xbox and AppleTV use very similar frame buffers.

#include <freeldr.h>


PVOID FrameBuffer;
ULONG ScreenWidth;
ULONG ScreenHeight;
ULONG ScreenPitch;
ULONG ScreenBPP;
ULONG ScreenDepth;
ULONG Delta;
ULONG ScreenBaseHack;

UCHAR MachDefaultTextColor = COLOR_GRAY;

BOOLEAN VideoIsInitialized = FALSE;

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

#define TOP_BOTTOM_LINES 0
#define MAKE_COLOR(Red, Green, Blue) (0xff000000 | (((Red) & 0xff) << 16) | (((Green) & 0xff) << 8) | ((Blue) & 0xff))

static VOID
AppleTVVideoOutputChar(UCHAR Char, unsigned X, unsigned Y, ULONG FgColor, ULONG BgColor)
{
    PUCHAR FontPtr;
    PULONG Pixel;
    UCHAR Mask;
    unsigned Line;
    unsigned Col;

    FontPtr = BitmapFont8x16 + Char * 16;
    Pixel = (PULONG) ((char *) FrameBuffer + (Y * CHAR_HEIGHT + TOP_BOTTOM_LINES) * Delta
                  + X * CHAR_WIDTH * ScreenBPP);
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

ULONG
AppleTVVideoGetBufferSize(VOID)
{
    return (ScreenHeight * ScreenPitch);
}

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


VOID
AppleTVVideoAttrToColors(UCHAR Attr, ULONG *FgColor, ULONG *BgColor)
{
    *FgColor = AppleTVVideoAttrToSingleColor(Attr & 0xf);
    *BgColor = AppleTVVideoAttrToSingleColor((Attr >> 4) & 0xf);
}

VOID
AppleTVVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{    
    ULONG FgColor, BgColor;

    AppleTVVideoAttrToColors(Attr, &FgColor, &BgColor);

    AppleTVVideoOutputChar(Ch, X, Y, FgColor, BgColor);
}


VOID
AppleTVVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    *Width = ScreenWidth / CHAR_WIDTH;
    *Height = (ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT;
    *Depth = 0;
}

VOID
AppleTVVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
    PUCHAR OffScreenBuffer = (PUCHAR) Buffer;
    ULONG Col, Line;
    for (Line = 0; Line < (ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT; Line++)
      {
        for (Col = 0; Col < ScreenWidth / CHAR_WIDTH; Col++)
            {
                AppleTVVideoPutChar(OffScreenBuffer[0], OffScreenBuffer[1], Col, Line);
                OffScreenBuffer += 2;
            }
      }
}




static VOID
AppleTVVideoClearScreenColor(ULONG Color, BOOLEAN FullScreen)
{
    ULONG Line, Col;
    PULONG p;

    for (Line = 0; Line < ScreenHeight - (FullScreen ? 0 : 2 * TOP_BOTTOM_LINES); Line++)
    {
        p = (PULONG) ((char *) FrameBuffer + (Line + (FullScreen ? 0 : TOP_BOTTOM_LINES)) * Delta);
            for (Col = 0; Col < ScreenWidth; Col++)
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
AppleTVVideoScrollUp(VOID)
{
    ULONG BgColor, Dummy;
    ULONG PixelCount = ScreenWidth * CHAR_HEIGHT *
                       (((ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT) - 1);
    PULONG Src = (PULONG)((PUCHAR)FrameBuffer + (CHAR_HEIGHT + TOP_BOTTOM_LINES) * Delta);
    PULONG Dst = (PULONG)((PUCHAR)FrameBuffer + TOP_BOTTOM_LINES * Delta);

    AppleTVVideoAttrToColors(ATTR(COLOR_WHITE, COLOR_BLACK), &Dummy, &BgColor);

    while (PixelCount--)
        *Dst++ = *Src++;

    for (PixelCount = 0; PixelCount < ScreenWidth * CHAR_HEIGHT; PixelCount++)
        *Dst++ = BgColor;
}

VOID
AppleTVVideoInit(VOID)
{
    /* Setup variables. */
    FrameBuffer = (PVOID) appletv_boot_info->video.base;
    ScreenWidth = (appletv_boot_info->video.pitch / 4);
    ScreenPitch = appletv_boot_info->video.pitch;
    ScreenHeight = appletv_boot_info->video.height;
    ScreenBPP = (appletv_boot_info->video.depth / 8);
    ScreenDepth = appletv_boot_info->video.depth;
    ScreenBaseHack = appletv_boot_info->video.base;

    Delta = (ScreenWidth * ScreenBPP + 3) & ~ 0x3;


}

VOID
AppleTVVideoPrepareForReactOS(VOID)
{
    AppleTVVideoHideShowTextCursor(FALSE);
}