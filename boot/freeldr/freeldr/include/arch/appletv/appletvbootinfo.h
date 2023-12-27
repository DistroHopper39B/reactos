/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Apple TV boot info
 * COPYRIGHT:   Copyright 2023 DistroHopper39B (distrohopper39b.business@gmail.com)
 */

#pragma once

// Apple TV Magic number: verifies that we loaded the struct correctly.
#define ATV_LOADER_MAGIC_NUMBER 0xBAADC0DE
// Freeldr magic number: symbolizes the start of the executable info structure. Must be within the first 8192 bytes, just like multiboot.
#define FREELDR_MAGIC_NUMBER 0x600DC0DE

// Boot info location
#define BOOTINFO_LOC 0x6000
#define CMDLINE_LOC 0x7000

// Multiboot/E820 memory map entry.
struct mmap_entry {
    UINT64 Address;
    UINT64 Length;
#define MULTIBOOT_MEMORY_AVAILABLE		1
#define MULTIBOOT_MEMORY_RESERVED		2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5
    UINT32 Type;
} __attribute__((packed));
typedef struct mmap_entry memory_map_entry;

// This struct goes at the beginning of freeldr and is used as a header for information, much like multiboot.
typedef struct {
    UINT32 MagicNumber; // Freeldr magic number.
    UINT32 LoadAddress; // Where to load freeldr into memory.
    UINT32 EntryPoint; // Where to start executing code.
} FLDRHEADER, *PFLDRHEADER;


typedef struct {
    UINT32 MagicNumber; /* FreeLoader magic number */

    UINT32 VideoBaseAddr; /* Base address of framebuffer */
    UINT32 VideoPitch; /* Bytes per row */
    UINT32 VideoWidth; /* Width of screen in pixels */
    UINT32 VideoHeight; /* Height of screen in pixels */
    UINT32 VideoDepth; /* Depth of screen */

    UINT32 MemoryMapAddr; /* Base address of Multiboot memory map */
    UINT32 MemoryMapEntries; /* Number of entries in multiboot memory map */
    UINT32 MemoryMapSize; /* Size of memory map */

    UINT32 GlobalSystemTable; /* EFI system table */
} BOOTINFO, *PBOOTINFO;