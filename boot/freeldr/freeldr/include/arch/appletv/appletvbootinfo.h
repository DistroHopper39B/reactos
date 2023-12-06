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
#define FREELDR_MAGIC_NUMBER 0xF00DBEEF

// EFI memory map information.
typedef struct {
    unsigned long addr;
    unsigned long size;
    unsigned long descriptor_size;
    unsigned long descriptor_version;
} efi_memory_map;

// Video mode information.
typedef struct {
    unsigned long base; // Address to write to for VRAM
    unsigned long pitch; // Bytes per row
    unsigned long width; // Width
    unsigned long height; // Height
    unsigned long depth; // Color depth
} video_attr;

// Kernel geometry information. Useful for the memory allocator later on.
typedef struct {
    unsigned long base_addr; // Base address of kernel as specified by linker argument "-segaddr __TEXT".
    unsigned long size; // Size of kernel + other memory we can't write to in userspace (such as UEFI BIOS information, memory maps, etc).
    unsigned long end; // End of kernel. Everything past this address should always be safe to write to by userspace.
} mach_environment_params;

// Multiboot/E820 memory map information.
typedef struct {
    unsigned long addr; // Pointer to memory map info
    unsigned long size; // Size of memory map
    unsigned long entries; // Number of entries in memory map
} memory_map;

// Multiboot/E820 memory map entry.
struct mmap_entry {
    unsigned long long addr;
    unsigned long long len;
#define MULTIBOOT_MEMORY_AVAILABLE		1
#define MULTIBOOT_MEMORY_RESERVED		2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5
    unsigned long type;
} __attribute__((packed));
typedef struct mmap_entry memory_map_entry;

typedef struct {
    unsigned long magic; // Apple TV magic number
    efi_memory_map efi_map; // EFI memory map information
    unsigned long efi_system_table_ptr; // Pointer to EFI system table.
    memory_map multiboot_map; // E820/Multiboot memory map information
    video_attr video; // Frame buffer information
    unsigned long cmdline_ptr; // Pointer to command line
    mach_environment_params kernel; // Kernel geometry.
} handoff_boot_info;

// This struct goes at the beginning of freeldr and is used as a header for information, much like multiboot.
typedef struct {
    unsigned long magic; // Freeldr magic number.
    unsigned long load_addr; // Where to load freeldr into memory.
    unsigned long entry_point; // Where to start executing code.
} freeldr_hdr;
