/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware-specific creating a memory map routine for the original Apple TV
 * COPYRIGHT:   Copyright 2023 DistroHopper39B (distrohopper39b.business@gmail.com)
 */

#include <freeldr.h>

#define hi32(a) ((UINT32)((a) >> 32))
#define lo32(a) ((UINT32)(a))

INT FreeldrDescCount;

FREELDR_MEMORY_DESCRIPTOR AppleTVMemoryMap[MAX_BIOS_DESCRIPTORS + 1];

PFREELDR_MEMORY_DESCRIPTOR
AppleTVMemGetMemoryMap(ULONG *MemoryMapSize)
{
    return NULL;
}