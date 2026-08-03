/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware-specific creating a memory map routine for the original Apple TV
 * COPYRIGHT:   Authors of uefimem.c and pcmem.c
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

ULONG FreeldrDescCount = 0;
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
        case EfiReservedMemoryType:
            return LoaderReserve;
        case EfiLoaderCode:
            return LoaderLoadedProgram;
        case EfiLoaderData:
            return LoaderLoadedProgram;
        case EfiBootServicesCode:
            return LoaderFirmwareTemporary;
        case EfiBootServicesData:
            return LoaderFirmwareTemporary;
        case EfiRuntimeServicesCode:
            return LoaderFirmwarePermanent;
        case EfiRuntimeServicesData:
            return LoaderFirmwarePermanent;
        case EfiConventionalMemory:
            return LoaderFree;
        case EfiUnusableMemory:
            return LoaderBad;
        case EfiACPIReclaimMemory:
            return LoaderSpecialMemory;
        case EfiACPIMemoryNVS:
            return LoaderSpecialMemory;
        case EfiMemoryMappedIO:
            return LoaderReserve;
        case EfiMemoryMappedIOPortSpace:
            return LoaderReserve;
        default:
            break;
    }
    return LoaderReserve;
}

static
VOID
AppleTVMemFinalizeMemoryMap(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap
)
{
    ULONG i;

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

    /* Set the memory range for the disk read buffer */
    UefiSetMemory(MemoryMap,
                  (ULONG_PTR)DiskReadBuffer,
                  EFI_SIZE_TO_PAGES(DiskReadBufferSize),
                  LoaderFirmwareTemporary);
}

PFREELDR_MEMORY_DESCRIPTOR
AppleTVMemGetMemoryMap(ULONG *MemoryMapSize)
{
    EFI_MEMORY_DESCRIPTOR   *EfiMemoryMap, *CurrentDescriptor;
    SIZE_T                  EfiMemoryMapSize, EfiMemoryDescriptorSize,
                            EfiNumberOfEntries, FreeldrMemMapSize;
    ULONG                   i;

    EfiMemoryMap            = (EFI_MEMORY_DESCRIPTOR *)BootArgs->EfiMemoryMap;
    EfiMemoryMapSize        = BootArgs->EfiMemoryMapSize;
    EfiMemoryDescriptorSize = BootArgs->EfiMemoryDescriptorSize;

    EfiNumberOfEntries = EfiMemoryMapSize / EfiMemoryDescriptorSize;

    /*
     * We add 4 extra entries here to compensate for the static locations
     * If we don't do this, the memory map may become corrupted resulting in a bugcheck.
     */
    FreeldrMemMapSize = (EfiNumberOfEntries + 4) * sizeof(FREELDR_MEMORY_DESCRIPTOR);

    /* Find a free space above the FreeLoader image for the memory map */
    CurrentDescriptor = EfiMemoryMap;
    for (i = 0; i < EfiNumberOfEntries; i++)
    {
        if (CurrentDescriptor->PhysicalStart > FREELDR_BASE + FrLdrImageSize &&
            CurrentDescriptor->NumberOfPages > FreeldrMemMapSize &&
            CurrentDescriptor->Type == EfiConventionalMemory)
        {
            /* We found where to put the memory map. */
            TRACE("Putting memory map @ 0x%X\n", CurrentDescriptor->PhysicalStart);
            FreeldrMemMap = (PFREELDR_MEMORY_DESCRIPTOR)((ULONG_PTR)CurrentDescriptor->PhysicalStart);
            break;
        }

        CurrentDescriptor = NEXT_MEMORY_DESCRIPTOR(CurrentDescriptor, EfiMemoryDescriptorSize);
    }

    ASSERT(FreeldrMemMap != NULL);
    RtlZeroMemory(FreeldrMemMap, FreeldrMemMapSize);

    UefiSetMemory(FreeldrMemMap,
                (ULONG_PTR)FreeldrMemMap,
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

    /* Reserve some static locations */
    /* First page */
    UefiSetMemory(FreeldrMemMap,
                  0x0,
                  1,
                  LoaderFirmwarePermanent);

    /* FreeLoader stack */
    UefiSetMemory(FreeldrMemMap,
                  STACKLOW,
                  EFI_SIZE_TO_PAGES(STACKADDR - STACKLOW),
                  LoaderOsloaderStack);

    /* FreeLoader program */
    UefiSetMemory(FreeldrMemMap,
                  FREELDR_BASE,
                  EFI_SIZE_TO_PAGES(FrLdrImageSize),
                  LoaderLoadedProgram);

    /* Allocate disk read buffer */
    AppleTVMemFinalizeMemoryMap(FreeldrMemMap);

    *MemoryMapSize = FreeldrDescCount;
    return FreeldrMemMap;
}
