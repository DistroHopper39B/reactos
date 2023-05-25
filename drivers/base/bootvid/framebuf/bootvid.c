/*
 * PROJECT:     ReactOS Generic Framebuffer Boot Video Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2023-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* Include the Boot-time (POST) display discovery helper functions */
#include <drivers/bootvid/framebuf.c>

/* Scaling of the bootvid 640x480 default virtual screen to the larger video framebuffer */
#define SCALING_SUPPORT
#define SCALING_PROPORTIONAL

/* Keep borders black or controlled with palette */
// #define COLORED_BORDERS


/* GLOBALS ********************************************************************/

#define BB_PIXEL(x, y)  ((PUCHAR)BackBuffer + (y) * SCREEN_WIDTH + (x))
#define FB_PIXEL(x, y)  ((PUCHAR)FrameBufferStart + ((PanV + VidpYScale * (y)) * FrameBufferWidth + PanH + VidpXScale * (x)) * BytesPerPixel)

static ULONG_PTR FrameBufferStart = 0;
static ULONG FrameBufferWidth, FrameBufferHeight, PanH, PanV;
static UCHAR BytesPerPixel;
static RGBQUAD CachedPalette[BV_MAX_COLORS];
static PUCHAR BackBuffer = NULL;

#ifdef SCALING_SUPPORT
static USHORT VidpXScale = 1;
static USHORT VidpYScale = 1;
#else
#define VidpXScale 1
#define VidpYScale 1
#endif

typedef struct _BOOT_DISPLAY_INFO
{
    PHYSICAL_ADDRESS BaseAddress; // ULONG_PTR
    ULONG BufferSize; // SIZE_T ?

    PVOID FrameAddress; // Mapped framebuffer virtual address.

    /* Configuration data from hardware tree */
    CM_FRAMEBUF_DEVICE_DATA VideoConfigData;

} BOOT_DISPLAY_INFO, *PBOOT_DISPLAY_INFO;

BOOT_DISPLAY_INFO gBootDisp = {0};

/* PRIVATE FUNCTIONS *********************************************************/

static VOID
ApplyPalette(VOID)
{
    PULONG Frame = (PULONG)FrameBufferStart;
    ULONG x, y;

#ifdef COLORED_BORDERS
    /* Top border */
    for (x = 0; x < PanV * FrameBufferWidth; ++x)
    {
        *Frame++ = CachedPalette[BV_COLOR_BLACK];
    }

    /* Left border */
    for (y = 0; y < VidpYScale * SCREEN_HEIGHT; ++y)
    {
        // Frame = (PULONG)(FrameBufferStart + FB_OFFSET(-(LONG)PanH, y));
        Frame = (PULONG)(FrameBufferStart + ((PanV + y) * FrameBufferWidth) * BytesPerPixel);
        for (x = 0; x < PanH; ++x)
        {
            *Frame++ = CachedPalette[BV_COLOR_BLACK];
        }
    }
#endif // COLORED_BORDERS

    /* Screen redraw */
    PUCHAR Back = BackBuffer;
    for (y = 0; y < SCREEN_HEIGHT; ++y)
    {
        Frame = (PULONG)FB_PIXEL(0, y);
        PULONG Pixel = Frame;
        for (x = 0; x < SCREEN_WIDTH; ++x)
        {
            for (ULONG j = VidpXScale; j > 0; --j)
                *Pixel++ = CachedPalette[*Back];
            Back++;
        }
        Pixel = Frame;
        for (ULONG i = VidpYScale-1; i > 0; --i)
        {
            Pixel = (PULONG)((ULONG_PTR)Pixel + FrameBufferWidth * BytesPerPixel);
            RtlCopyMemory(Pixel, Frame, VidpXScale * SCREEN_WIDTH * BytesPerPixel);
        }
    }

#ifdef COLORED_BORDERS
    /* Right border */
    for (y = 0; y < VidpYScale * SCREEN_HEIGHT; ++y)
    {
        // Frame = (PULONG)(FrameBufferStart + FB_OFFSET(SCREEN_WIDTH, y));
        Frame = (PULONG)(FrameBufferStart + ((PanV + y) * FrameBufferWidth + PanH + (VidpXScale * SCREEN_WIDTH)) * BytesPerPixel);
        for (x = 0; x < PanH; ++x)
        {
            *Frame++ = CachedPalette[BV_COLOR_BLACK];
        }
    }

    /* Bottom border */
    // Frame = (PULONG)(FrameBufferStart + FB_OFFSET(-(LONG)PanH, SCREEN_HEIGHT));
    Frame = (PULONG)(FrameBufferStart + ((PanV + VidpYScale * SCREEN_HEIGHT) * FrameBufferWidth) * BytesPerPixel);
    for (x = 0; x < PanV * FrameBufferWidth; ++x)
    {
        *Frame++ = CachedPalette[BV_COLOR_BLACK];
    }
#endif // COLORED_BORDERS
}

/* PUBLIC FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
VidInitialize(
    _In_ BOOLEAN SetMode)
{
    NTSTATUS Status;
    INTERFACE_TYPE Interface;
    ULONG BusNumber;

__debugbreak();
    /* Find boot-time (POST) framebuffer display information from
     * LoaderBlock or BgContext */
    RtlZeroMemory(&gBootDisp, sizeof(gBootDisp));
    Status = FindBootDisplay(&gBootDisp.BaseAddress,
                             &gBootDisp.BufferSize,
                             &gBootDisp.VideoConfigData,
                             NULL, // MonitorConfigData
                             &Interface,  // FIXME: Make it opt?
                             &BusNumber); // FIXME: Make it opt?
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Boot framebuffer does not exist!\n");
        return FALSE;
    }


    PHYSICAL_ADDRESS FrameBuffer = gBootDisp.BaseAddress;
    FrameBufferWidth  = gBootDisp.VideoConfigData.ScreenWidth;
    FrameBufferHeight = gBootDisp.VideoConfigData.ScreenHeight;

    /* Verify that the framebuffer address is page-aligned */
    ASSERT(FrameBuffer.QuadPart % PAGE_SIZE == 0);

    if (FrameBufferWidth < SCREEN_WIDTH || FrameBufferHeight < SCREEN_HEIGHT)
    {
        DPRINT1("Unsupported screen resolution!\n");
        goto Failure;
    }

    BytesPerPixel = (gBootDisp.VideoConfigData.BitsPerPixel / 8);
    ASSERT(BytesPerPixel >= 1 && BytesPerPixel <= 4);

    if (BytesPerPixel != 4)
    {
        DPRINT1("Unsupported BytesPerPixel = %d\n", BytesPerPixel);
        goto Failure;
    }

    /* Verify that screen fits framebuffer size */
    ULONG FrameBufferSize = FrameBufferWidth * FrameBufferHeight * BytesPerPixel;
    if (FrameBufferSize > gBootDisp.BufferSize)
    {
        DPRINT1("Current screen resolution exceeds video memory bounds!\n");
        goto Failure;
    }

#ifdef SCALING_SUPPORT
    /* Compute autoscaling; only integer (not fractional) scaling is supported */
    VidpXScale = FrameBufferWidth / SCREEN_WIDTH;
    VidpYScale = FrameBufferHeight / SCREEN_HEIGHT;
    ASSERT(VidpXScale >= 1);
    ASSERT(VidpYScale >= 1);
#ifdef SCALING_PROPORTIONAL
    VidpXScale = min(VidpXScale, VidpYScale);
    VidpYScale = VidpXScale;
#endif
#endif // SCALING_SUPPORT

    /* Calculate border values */
    PanH = (FrameBufferWidth - VidpXScale * SCREEN_WIDTH) / 2;
    PanV = (FrameBufferHeight - VidpYScale * SCREEN_HEIGHT) / 2;

    /* Convert from bus-relative to physical address, and map it into system space */
    PHYSICAL_ADDRESS TranslatedAddress;
    ULONG AddressSpace = 0;
    if (!BootTranslateBusAddress(Interface,
                                 BusNumber,
                                 FrameBuffer,
                                 &AddressSpace,
                                 &TranslatedAddress))
    {
        DPRINT1("Could not map 0x%p\n", FrameBuffer.QuadPart);
        goto Failure;
    }

    if (AddressSpace == 0)
    {
        FrameBufferStart = (ULONG_PTR)MmMapIoSpace(TranslatedAddress,
                                                   gBootDisp.BufferSize,
                                                   MmNonCached);
        if (!FrameBufferStart)
        {
            DPRINT1("Out of memory!\n");
            goto Failure;
        }
    }
    else
    {
        /* The base is the translated address, no need to map */
        FrameBufferStart = (ULONG_PTR)TranslatedAddress.LowPart; // Yes, LowPart.
    }


    /*
     * Reserve off-screen area for the backbuffer that contains
     * 8-bit indexed color screen image, plus preserved row data.
     */
    SIZE_T BackBufferSize = SCREEN_WIDTH * (SCREEN_HEIGHT + (BOOTCHAR_HEIGHT + 1));

    /* If there is enough video memory in the physical framebuffer,
     * place the backbuffer in the hidden part of the framebuffer,
     * otherwise allocate a zone for the backbuffer. */
    if (gBootDisp.BufferSize >= FrameBufferSize + BackBufferSize)
    {
        /* Backbuffer placed in the framebuffer hidden part */
        BackBuffer = (PUCHAR)(FrameBufferStart + gBootDisp.BufferSize - BackBufferSize);
    }
    else // if (gBootDisp.BufferSize - FrameBufferSize < BackBufferSize)
    {
        /* Allocate the backbuffer */
        // PHYSICAL_ADDRESS PhysicalAddress = RTL_CONSTANT_LARGE_INTEGER(-1LL);
        // BackBuffer = MmAllocateContiguousMemory(BackBufferSize, PhysicalAddress);
        BackBuffer = ExAllocatePoolWithTag(NonPagedPool, BackBufferSize, 'bfGB');
        if (!BackBuffer)
        {
            DPRINT1("Out of memory!\n");
            goto Failure;
        }
    }

    /* Reset the video mode if requested */
    if (SetMode)
        VidResetDisplay(TRUE);

    return TRUE;

Failure:
    /* We failed somewhere. Unmap the I/O space if we mapped it */
    if (FrameBufferStart && (AddressSpace == 0))
        MmUnmapIoSpace((PVOID)FrameBufferStart, gBootDisp.BufferSize);

    return FALSE;
}

VOID
NTAPI
VidCleanUp(VOID)
{
    /* Just fill the screen black */
    VidSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_BLACK);
}

VOID
ResetDisplay(
    _In_ BOOLEAN SetMode)
{
    SIZE_T BackBufferSize = SCREEN_WIDTH * (SCREEN_HEIGHT + (BOOTCHAR_HEIGHT + 1));

    // RtlZeroMemory(BackBuffer, BackBufferSize);
    RtlFillMemory(BackBuffer, BackBufferSize, 0xAA); // FIXME: Testing purposes!

    // RtlZeroMemory((PVOID)FrameBufferStart, gBootDisp.BufferSize);
    RtlFillMemoryUlong((PVOID)FrameBufferStart, gBootDisp.BufferSize, 0x00FFCC); // FIXME: Testing purposes!

    /* Re-initialize the palette and fill the screen black */
    InitializePalette();
    VidSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_BLACK);
}

VOID
InitPaletteWithTable(
    _In_reads_(Count) const ULONG* Table,
    _In_ ULONG Count)
{
    const ULONG* Entry = Table;
    ULONG i;

    for (i = 0; i < Count; i++, Entry++)
    {
        CachedPalette[i] = *Entry | 0xFF000000;
    }
    ApplyPalette();
}

VOID
SetPixel(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ UCHAR Color)
{
    PUCHAR Back = BB_PIXEL(Left, Top);
    PULONG Frame = (PULONG)FB_PIXEL(Left, Top);

    *Back = Color;
    for (ULONG i = VidpYScale; i > 0; --i)
    {
        PULONG Pixel = Frame;
        for (ULONG j = VidpXScale; j > 0; --j)
            *Pixel++ = CachedPalette[Color];
        Frame = (PULONG)((ULONG_PTR)Frame + FrameBufferWidth * BytesPerPixel);
    }
}

VOID
PreserveRow(
    _In_ ULONG CurrentTop,
    _In_ ULONG TopDelta,
    _In_ BOOLEAN Restore)
{
    PUCHAR NewPosition, OldPosition;

    /* Calculate the position in memory for the row */
    if (Restore)
    {
        /* Restore the row by copying back the contents saved off-screen */
        NewPosition = BB_PIXEL(0, CurrentTop);
        OldPosition = BB_PIXEL(0, SCREEN_HEIGHT);
    }
    else
    {
        /* Preserve the row by saving its contents off-screen */
        NewPosition = BB_PIXEL(0, SCREEN_HEIGHT);
        OldPosition = BB_PIXEL(0, CurrentTop);
    }

    /* Set the count and copy the pixel data back to the other position in the backbuffer */
    ULONG Count = TopDelta * SCREEN_WIDTH;
    RtlCopyMemory(NewPosition, OldPosition, Count);

    /* On restore, mirror the backbuffer changes to the framebuffer */
    if (Restore)
    {
        NewPosition = BB_PIXEL(0, CurrentTop);
        for (ULONG y = 0; y < TopDelta; ++y)
        {
            PULONG Frame = (PULONG)FB_PIXEL(0, CurrentTop + y);
            PULONG Pixel = Frame;
            for (Count = 0; Count < SCREEN_WIDTH; ++Count)
            {
                for (ULONG j = VidpXScale; j > 0; --j)
                    *Pixel++ = CachedPalette[*NewPosition];
                NewPosition++;
            }
            Pixel = Frame;
            for (ULONG i = VidpYScale-1; i > 0; --i)
            {
                Pixel = (PULONG)((ULONG_PTR)Pixel + FrameBufferWidth * BytesPerPixel);
                RtlCopyMemory(Pixel, Frame, VidpXScale * SCREEN_WIDTH * BytesPerPixel);
            }
        }
    }
}

VOID
DoScroll(
    _In_ ULONG Scroll)
{
    ULONG RowSize = VidpScrollRegion.Right - VidpScrollRegion.Left + 1;

    /* Calculate the position in memory for the row */
    PUCHAR OldPosition = BB_PIXEL(VidpScrollRegion.Left, VidpScrollRegion.Top + Scroll);
    PUCHAR NewPosition = BB_PIXEL(VidpScrollRegion.Left, VidpScrollRegion.Top);

    /* Start loop */
    for (ULONG Top = VidpScrollRegion.Top; Top <= VidpScrollRegion.Bottom; ++Top)
    {
        /* Scroll the row */
        RtlCopyMemory(NewPosition, OldPosition, RowSize);

        PULONG Frame = (PULONG)FB_PIXEL(VidpScrollRegion.Left, Top);
        PULONG Pixel = Frame;
        for (ULONG Count = 0; Count < RowSize; ++Count)
        {
            for (ULONG j = VidpXScale; j > 0; --j)
                *Pixel++ = CachedPalette[NewPosition[Count]];
        }
        Pixel = Frame;
        for (ULONG i = VidpYScale-1; i > 0; --i)
        {
            Pixel = (PULONG)((ULONG_PTR)Pixel + FrameBufferWidth * BytesPerPixel);
            RtlCopyMemory(Pixel, Frame, VidpXScale * RowSize * BytesPerPixel);
        }

        OldPosition += SCREEN_WIDTH;
        NewPosition += SCREEN_WIDTH;
    }
}

VOID
DisplayCharacter(
    _In_ CHAR Character,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG TextColor,
    _In_ ULONG BackColor)
{
    /* Get the font line for this character */
    const UCHAR* FontChar = GetFontPtr(Character);

    /* Loop each pixel height */
    for (ULONG y = Top; y < Top + BOOTCHAR_HEIGHT; ++y, FontChar += FONT_PTR_DELTA)
    {
        /* Loop each pixel width */
        ULONG x = Left;
        for (UCHAR bit = 1 << (BOOTCHAR_WIDTH - 1); bit > 0; bit >>= 1, ++x)
        {
            /* If we should draw this pixel, use the text color. Otherwise
             * this is a background pixel, draw it unless it's transparent. */
            if (*FontChar & bit)
                SetPixel(x, y, (UCHAR)TextColor);
            else if (BackColor < BV_COLOR_NONE)
                SetPixel(x, y, (UCHAR)BackColor);
        }
    }
}

VOID
NTAPI
VidSolidColorFill(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ UCHAR Color)
{
    for (; Top <= Bottom; ++Top)
    {
        PUCHAR Back = BB_PIXEL(Left, Top);
        // NOTE: Assumes 32bpp
        PULONG Frame = (PULONG)FB_PIXEL(Left, Top);
        PULONG Pixel = Frame;
        for (ULONG L = Left; L <= Right; ++L)
        {
            *Back++ = Color;
            for (ULONG j = VidpXScale; j > 0; --j)
                *Pixel++ = CachedPalette[Color];
        }
        Pixel = Frame;
        for (ULONG i = VidpYScale-1; i > 0; --i)
        {
            Pixel = (PULONG)((ULONG_PTR)Pixel + FrameBufferWidth * BytesPerPixel);
            RtlCopyMemory(Pixel, Frame, VidpXScale * (Right - Left + 1) * BytesPerPixel);
        }
    }
}

VOID
NTAPI
VidScreenToBufferBlt(
    _Out_writes_bytes_all_(Delta * Height) PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    ULONG x, y;

    /* Clear the destination buffer */
    RtlZeroMemory(Buffer, Delta * Height);

    /* Start the outer Y height loop */
    for (y = 0; y < Height; ++y)
    {
        /* Set current scanline */
        PUCHAR Back = BB_PIXEL(Left, Top + y);
        PUCHAR Buf = Buffer + y * Delta;

        /* Start the X inner loop */
        for (x = 0; x < Width; x += sizeof(USHORT))
        {
            /* Read the current value */
            *Buf = (*Back++ & 0xF) << 4;
            *Buf |= *Back++ & 0xF;
            Buf++;
        }
    }
}

/* EOF */
