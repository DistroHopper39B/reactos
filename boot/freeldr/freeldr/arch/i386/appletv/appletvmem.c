/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware-specific creating a memory map routine for the original Apple TV
 * COPYRIGHT:   Copyright 2023 DistroHopper39B (distrohopper39b.business@gmail.com)
 */

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(MEMORY);

#define hi32(a) ((UINT32)((a) >> 32))
#define lo32(a) ((UINT32)(a))

// external functions from pcmem.c
extern VOID
SetMemory(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    ULONG_PTR BaseAddress,
    SIZE_T Size,
    TYPE_OF_MEMORY MemoryType);

extern VOID
ReserveMemory(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    ULONG_PTR BaseAddress,
    SIZE_T Size,
    TYPE_OF_MEMORY MemoryType,
    PCHAR Usage);

extern ULONG
PcMemFinalizeMemoryMap(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap);

/* Get memory map passed in from atv-playground.
 * This memory map is in multiboot format (see ../xbox/xboxmem.c)
 */
memory_map_entry *
AppleTVGetMultibootMemoryMap(INT *Count)
{
    memory_map_entry* MemoryMap;
    MemoryMap = (memory_map_entry *) appletv_boot_info->multiboot_map.addr;
    *Count = appletv_boot_info->multiboot_map.entries;
    return MemoryMap;
}

/* Choose memory type. */
TYPE_OF_MEMORY
AppleTVMultibootMemoryType(ULONG Type)
{
    switch (Type)
    {
        case MULTIBOOT_MEMORY_AVAILABLE: // Free memory
            return LoaderFree;
        case MULTIBOOT_MEMORY_RESERVED: // Reserved memory
            return LoaderFirmwarePermanent;
        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE: // Reclaimable ACPI memory
            return LoaderFirmwareTemporary;
        case MULTIBOOT_MEMORY_NVS: // Non-reclaimable ACPI memory
            return LoaderFirmwarePermanent;
        default:
            return LoaderFirmwarePermanent;
    }
}


FREELDR_MEMORY_DESCRIPTOR AppleTVMemoryMap[MAX_BIOS_DESCRIPTORS + 1];

PFREELDR_MEMORY_DESCRIPTOR
AppleTVMemGetMemoryMap(ULONG *MemoryMapSize)
{
    memory_map_entry* MbMap;
    INT Count, i;

    TRACE("AppleTVMemGetMemoryMap()\n");

    MbMap = AppleTVGetMultibootMemoryMap(&Count);

    for (i = 0; i < Count; i++, MbMap++)
    {
        TRACE("i = %d, addr = 0x%08X%08X, len = 0x%08X%08X, type = %i\n", i, hi32(MbMap->addr), lo32(MbMap->addr), hi32(MbMap->len), lo32(MbMap->len), MbMap->type);
        SetMemory(AppleTVMemoryMap,
                MbMap->addr,
                MbMap->len,
                AppleTVMultibootMemoryType(MbMap->type));
    }
    SetMemory(AppleTVMemoryMap, 0x000000, 0x01000, LoaderFirmwarePermanent); // Realmode IVT / BDA
    SetMemory(AppleTVMemoryMap, 0x0A0000, 0x50000, LoaderFirmwarePermanent); // Video memory
    SetMemory(AppleTVMemoryMap, 0x0F0000, 0x10000, LoaderSpecialMemory); // ROM
    SetMemory(AppleTVMemoryMap, 0xFFF000, 0x01000, LoaderSpecialMemory); // unusable memory (do we really need this?)
    
    *MemoryMapSize = PcMemFinalizeMemoryMap(AppleTVMemoryMap);
    return AppleTVMemoryMap;
}