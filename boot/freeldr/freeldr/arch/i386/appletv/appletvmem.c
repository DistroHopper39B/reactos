/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware-specific creating a memory map routine for the original Apple TV
 * COPYRIGHT:   Authors of uefimem.c
 *              Copyright 2023-2026 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>
#include <Uefi.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

/* GLOBALS *******************************************************************/

#define NEXT_MEMORY_DESCRIPTOR(Descriptor, DescriptorSize) \
    (EFI_MEMORY_DESCRIPTOR*)((char*)(Descriptor) + (DescriptorSize))
#define UNUSED_MAX_DESCRIPTOR_COUNT 10000

ULONG
AddMemoryDescriptor(
    IN OUT PFREELDR_MEMORY_DESCRIPTOR List,
    IN ULONG MaxCount,
    IN PFN_NUMBER BasePage,
    IN PFN_NUMBER PageCount,
    IN TYPE_OF_MEMORY MemoryType);

INT FreeldrDescCount = 0;
PFREELDR_MEMORY_DESCRIPTOR FreeldrMemMap = NULL;

/* FUNCTIONS *****************************************************************/

static
VOID
UefiSetMemory(
    _Inout_ PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    _In_ ULONG_PTR BaseAddress,
    _In_ PFN_COUNT SizeInPages,
    _In_ TYPE_OF_MEMORY MemoryType)
{
    ULONG_PTR BasePage, PageCount;

    BasePage = BaseAddress / EFI_PAGE_SIZE;
    PageCount = SizeInPages;

    /* Add the memory descriptor */
    FreeldrDescCount = AddMemoryDescriptor(MemoryMap,
                                           UNUSED_MAX_DESCRIPTOR_COUNT,
                                           BasePage,
                                           PageCount,
                                           MemoryType);
}

static
TYPE_OF_MEMORY
UefiConvertToFreeldrDesc(EFI_MEMORY_TYPE EfiMemoryType)
{
    switch (EfiMemoryType)
    {
        case EfiBootServicesCode:
        case EfiBootServicesData:
        case EfiLoaderCode:
        case EfiLoaderData:
            return LoaderFirmwareTemporary;
        case EfiConventionalMemory:
            return LoaderFree;
        case EfiReservedMemoryType:
        case EfiMemoryMappedIO:
        case EfiMemoryMappedIOPortSpace:
            return LoaderReserve;
        case EfiUnusableMemory:
            return LoaderBad;
        default:
            return LoaderSpecialMemory;
    }
}

PFREELDR_MEMORY_DESCRIPTOR
AppleTVMemGetMemoryMap(ULONG *MemoryMapSize)
{
    EFI_MEMORY_DESCRIPTOR   *EfiMemoryMap;
    EFI_MEMORY_DESCRIPTOR   *CurrentDescriptor;
    SIZE_T                  EfiMemoryMapSize;
    SIZE_T                  EfiMemoryDescriptorSize;
    SIZE_T                  EfiNumberOfEntries;
    SIZE_T                  FreeldrMemMapSize;
    ULONG                   i;
        
    EfiMemoryMap            = (EFI_MEMORY_DESCRIPTOR *) BootArgs->EfiMemoryMap;
    EfiMemoryMapSize        = BootArgs->EfiMemoryMapSize;
    EfiMemoryDescriptorSize = BootArgs->EfiMemoryDescriptorSize;
    
    /*
     * The number of FreeLoader entries tends to be higher than the number of EFI entries.
     * This can result in not enough space being allocated for the memory map, which causes
     * an instant bugcheck.
     * We multiply the size by 2 to compensate for this. This is a hack and should be replaced.
     */
    EfiNumberOfEntries = (EfiMemoryMapSize / EfiMemoryDescriptorSize);
    FreeldrMemMapSize = (EfiNumberOfEntries * sizeof(FREELDR_MEMORY_DESCRIPTOR)) * 2;
    
    // Find a free space above the FreeLoader image for the memory map
    CurrentDescriptor = EfiMemoryMap;
    for (i = 0; i < EfiNumberOfEntries; i++)
    {
        if (CurrentDescriptor->PhysicalStart > FREELDR_BASE + FrLdrImageSize &&
            CurrentDescriptor->NumberOfPages > FreeldrMemMapSize &&
            CurrentDescriptor->Type == EfiConventionalMemory)
        {
            // We found where to put the memory map.
            TRACE("Putting memory map @ 0x%X\n", CurrentDescriptor->PhysicalStart);
            FreeldrMemMap = (PFREELDR_MEMORY_DESCRIPTOR)((ULONG_PTR) CurrentDescriptor->PhysicalStart);
            break;
        }
        
        CurrentDescriptor = NEXT_MEMORY_DESCRIPTOR(CurrentDescriptor, EfiMemoryDescriptorSize);
    }
    
    ASSERT(FreeldrMemMap != NULL);
    
    RtlZeroMemory(FreeldrMemMap, FreeldrMemMapSize);
    
    UefiSetMemory(FreeldrMemMap,
                (ULONG_PTR) FreeldrMemMap,
                EFI_SIZE_TO_PAGES(FreeldrMemMapSize),
                LoaderSpecialMemory);
    
    CurrentDescriptor = EfiMemoryMap;
    for (i = 0; i < EfiNumberOfEntries; i++)
    {
        TYPE_OF_MEMORY MemoryType = UefiConvertToFreeldrDesc(CurrentDescriptor->Type);
        if (MemoryType != LoaderReserve)
        {
            UefiSetMemory(FreeldrMemMap,
                    CurrentDescriptor->PhysicalStart,
                    CurrentDescriptor->NumberOfPages,
                    MemoryType);
        }
        
        CurrentDescriptor = NEXT_MEMORY_DESCRIPTOR(CurrentDescriptor, EfiMemoryDescriptorSize);
    }
    
    // Reserve a few static ranges here
    // First page
    UefiSetMemory(FreeldrMemMap,
                0x0,
                1,
                LoaderFirmwarePermanent);
        
    // FreeLoader stack
    UefiSetMemory(FreeldrMemMap,
                STACKLOW,
                EFI_SIZE_TO_PAGES(STACKADDR - STACKLOW),
                LoaderOsloaderStack);
                
    // FreeLoader program
    UefiSetMemory(FreeldrMemMap,
                FREELDR_BASE,
                EFI_SIZE_TO_PAGES(FrLdrImageSize),
                LoaderLoadedProgram);
                
    *MemoryMapSize = FreeldrDescCount;
        
    return FreeldrMemMap;
}