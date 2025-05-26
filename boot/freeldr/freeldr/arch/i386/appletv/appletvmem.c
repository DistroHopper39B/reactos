/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware-specific creating a memory map routine for the original Apple TV
 * COPYRIGHT:   Copyright 2023-2025 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>
#include <Uefi.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

#define NEXT_MEMORY_DESCRIPTOR(Descriptor, DescriptorSize) \
    (EFI_MEMORY_DESCRIPTOR*)((char*)(Descriptor) + (DescriptorSize))

#define UNUSED_MAX_DESCRIPTOR_COUNT 10000

/* GLOBALS *******************************************************************/

ULONG
AddMemoryDescriptor(
    IN OUT PFREELDR_MEMORY_DESCRIPTOR List,
    IN ULONG MaxCount,
    IN PFN_NUMBER BasePage,
    IN PFN_NUMBER PageCount,
    IN TYPE_OF_MEMORY MemoryType);

INT FreeldrDescCount;
PFREELDR_MEMORY_DESCRIPTOR FreeldrMemMap = NULL;

/* FUNCTIONS *****************************************************************/

VOID
ReserveMemory(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    ULONG_PTR BaseAddress,
    SIZE_T Size,
    TYPE_OF_MEMORY MemoryType,
    PCHAR Usage)
{
    ULONG_PTR BasePage, PageCount;
    ULONG i;

    BasePage = BaseAddress / PAGE_SIZE;
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, Size);

    for (i = 0; i < FreeldrDescCount; i++)
    {
        /* Check for conflicting descriptor */
        if ((MemoryMap[i].BasePage < BasePage + PageCount) &&
            (MemoryMap[i].BasePage + MemoryMap[i].PageCount > BasePage))
        {
            /* Check if the memory is free */
            if (MemoryMap[i].MemoryType != LoaderFree)
            {
                FrLdrBugCheckWithMessage(
                    MEMORY_INIT_FAILURE,
                    __FILE__,
                    __LINE__,
                    "Failed to reserve memory in the range 0x%Ix - 0x%Ix for %s",
                    BaseAddress,
                    BaseAddress + Size,
                    Usage);
            }
        }
    }

    /* Add the memory descriptor */
    FreeldrDescCount = AddMemoryDescriptor(MemoryMap,
                                        UNUSED_MAX_DESCRIPTOR_COUNT,
                                        BasePage,
                                        PageCount,
                                        MemoryType);
}

VOID
SetMemory(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    ULONG_PTR BaseAddress,
    SIZE_T Size,
    TYPE_OF_MEMORY MemoryType)
{
    ULONG_PTR BasePage, PageCount;

    BasePage = BaseAddress / PAGE_SIZE;
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, Size);

    /* Add the memory descriptor */
    FreeldrDescCount = AddMemoryDescriptor(MemoryMap,
                                        UNUSED_MAX_DESCRIPTOR_COUNT,
                                        BasePage,
                                        PageCount,
                                        MemoryType);
}

ULONG
PcMemFinalizeMemoryMap(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap)
{
    ULONG i;

    /* Reserve some static ranges for freeldr */
    ReserveMemory(MemoryMap,
        0x1000,
        STACKLOW - 0x1000,
        LoaderFirmwareTemporary,
        "BIOS area");
    ReserveMemory(MemoryMap,
        STACKLOW,
        STACKADDR - STACKLOW,
        LoaderOsloaderStack,
        "FreeLdr stack");
    ReserveMemory(MemoryMap,
        FREELDR_BASE,
        FrLdrImageSize,
        LoaderLoadedProgram,
        "FreeLdr image");

    /* Default to 1 page above freeldr for the disk read buffer */
    DiskReadBuffer = (PUCHAR)ALIGN_UP_BY(FREELDR_BASE + FrLdrImageSize, PAGE_SIZE);
    DiskReadBufferSize = PAGE_SIZE;

    /* Scan for free range above freeldr image */
    for (i = 0; i < FreeldrDescCount; i++)
    {
        if ((MemoryMap[i].BasePage > (FREELDR_BASE / PAGE_SIZE)) &&
            (MemoryMap[i].MemoryType == LoaderFree))
        {
            /* Use this range for the disk read buffer */
            DiskReadBuffer = (PVOID)(MemoryMap[i].BasePage * PAGE_SIZE);
            DiskReadBufferSize = min(MemoryMap[i].PageCount * PAGE_SIZE,
                                     MAX_DISKREADBUFFER_SIZE);
            break;
        }
    }

    TRACE("DiskReadBuffer=0x%p, DiskReadBufferSize=0x%lx\n",
          DiskReadBuffer, DiskReadBufferSize);

    ASSERT(DiskReadBufferSize > 0);

    /* Now reserve the range for the disk read buffer */
    ReserveMemory(MemoryMap,
        (ULONG_PTR)DiskReadBuffer,
        DiskReadBufferSize,
        LoaderFirmwareTemporary,
        "Disk read buffer");

    TRACE("Dumping resulting memory map:\n");
    for (i = 0; i < FreeldrDescCount; i++)
    {
        TRACE("BasePage=0x%lx, PageCount=0x%lx, Type=%s\n",
              MemoryMap[i].BasePage,
              MemoryMap[i].PageCount,
              MmGetSystemMemoryMapTypeString(MemoryMap[i].MemoryType));
    }
    return FreeldrDescCount;
}

static
TYPE_OF_MEMORY UefiConvertToFreeldrType(EFI_MEMORY_TYPE MemoryType)
{
    switch (MemoryType)
    {
        case EfiBootServicesCode:
        case EfiBootServicesData:
        case EfiConventionalMemory:
        case EfiLoaderCode:
        case EfiLoaderData:
            return LoaderFree;
        case EfiUnusableMemory:
            return LoaderBad;
        default:
            return LoaderSpecialMemory;
    }
}

static
VOID
UefiConvertToFreeldrMemoryMap(EFI_MEMORY_DESCRIPTOR *EfiMemoryMapStart,
    SIZE_T MemoryMapSize,
    SIZE_T MemoryDescriptorSize)
{
    UINTN   EfiNumberOfEntries;
    UINTN   FreeldrMemMapSize;
    UINTN   i;
    TYPE_OF_MEMORY MemoryType;
    EFI_MEMORY_DESCRIPTOR *EfiMemoryMap = NULL;
    
    ASSERT(!FreeldrMemMap);
    
    EfiMemoryMap = EfiMemoryMapStart; // NEXT_MEMORY_DESCRIPTOR will change EfiMemoryMap's value
    EfiNumberOfEntries  = (MemoryMapSize / MemoryDescriptorSize);
    
    // The number of FreeLoader entries tends to be higher than the number of EFI entries.
    // We multiply the size by 2 to compensate for this.
    // This is a hack.
    FreeldrMemMapSize   = (EfiNumberOfEntries * sizeof(FREELDR_MEMORY_DESCRIPTOR)) * 2;
    
    TRACE("Number of entries: %d\n", EfiNumberOfEntries);
    TRACE("Bytes to be occupied by memory map: %d\n", FreeldrMemMapSize);
    
    // Find enough free memory for the memory map
    // We don't preallocate this because the memory map can be several hundred kilobytes in theory
    for (i = 0; i < EfiNumberOfEntries; i++)
    {    
        if (EfiMemoryMap->PhysicalStart != 0x0 && // don't put the memory map at NULL
            EfiMemoryMap->Type == EfiConventionalMemory &&
            (EfiMemoryMap->NumberOfPages << EFI_PAGE_SHIFT) >= FreeldrMemMapSize)
        {
            TRACE("Found enough memory! Map will be placed @ 0x%X\n", EfiMemoryMap->PhysicalStart);
            FreeldrMemMap = (PFREELDR_MEMORY_DESCRIPTOR) ((ULONG_PTR) EfiMemoryMap->PhysicalStart);
            break;
        }
        
        EfiMemoryMap = NEXT_MEMORY_DESCRIPTOR(EfiMemoryMap, MemoryDescriptorSize);
    }
    
    // Fail if there's somehow not enough RAM for this operation
    if (!FreeldrMemMap)
    {
        // fatal error
        FrLdrBugCheckWithMessage(MEMORY_INIT_FAILURE,
            __FILE__,
            __LINE__,
            "Unable to find %d bytes for FreeLoader memory map!",
            FreeldrMemMapSize);
    }
    
    // Zero out the memory map
    RtlZeroMemory(FreeldrMemMap, FreeldrMemMapSize);
    
    // Start at the beginning of the map again
    EfiMemoryMap = EfiMemoryMapStart;
    
    // Convert the UEFI map to a FreeLoader one, placing the new map in the location we just allocated
    for (i = 0; i < EfiNumberOfEntries; i++)
    { 
        MemoryType = UefiConvertToFreeldrType(EfiMemoryMap->Type);
        
        SetMemory(FreeldrMemMap,
            EfiMemoryMap->PhysicalStart,
            (EfiMemoryMap->NumberOfPages << EFI_PAGE_SHIFT),
            MemoryType);
        
        EfiMemoryMap = NEXT_MEMORY_DESCRIPTOR(EfiMemoryMap, MemoryDescriptorSize);
    }
    
    // Ensure the memory map isn't overwritten by anything
    ReserveMemory(FreeldrMemMap,
        (ULONG_PTR) FreeldrMemMap,
        FreeldrMemMapSize,
        LoaderFirmwareTemporary,
    "FreeLoader Memory Map");
}

PFREELDR_MEMORY_DESCRIPTOR
AppleTVMemGetMemoryMap(ULONG *MemoryMapSize)
{
    EFI_MEMORY_DESCRIPTOR   *EfiMemoryMap;
    SIZE_T                  EfiMemoryMapSize;
    SIZE_T                  EfiMemoryDescriptorSize;
    
    
    // Convert EFI memory map to BIOS memory map.
    EfiMemoryMap            = (EFI_MEMORY_DESCRIPTOR *) BootArgs->EfiMemoryMap;
    EfiMemoryMapSize        = BootArgs->EfiMemoryMapSize;
    EfiMemoryDescriptorSize = BootArgs->EfiMemoryDescriptorSize;
    
    UefiConvertToFreeldrMemoryMap(EfiMemoryMap,
                                        EfiMemoryMapSize,
                                        EfiMemoryDescriptorSize);
    
    // reserve some ranges to prevent windows bugs
    SetMemory(FreeldrMemMap,
        0x000000,
        0x01000,
        LoaderFirmwarePermanent); // Realmode IVT / BDA
    SetMemory(FreeldrMemMap,
        0x0A0000,
        0x50000,
        LoaderFirmwarePermanent); // Video memory
    SetMemory(FreeldrMemMap,
        0x0F0000,
        0x10000,
        LoaderSpecialMemory); // ROM
    SetMemory(FreeldrMemMap,
        0xFFF000,
        0x01000,
        LoaderSpecialMemory); // unusable memory (do we really need this?)
    
    *MemoryMapSize = PcMemFinalizeMemoryMap(FreeldrMemMap);
    
    // Prevent BootArgs from being overwritten (can this even happen?)
    SetMemory(FreeldrMemMap,
        (ULONG_PTR)BootArgs,
        sizeof(MACH_BOOTARGS),
        LoaderFirmwareTemporary);
    
    return FreeldrMemMap;
}