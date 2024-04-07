/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware-specific creating a memory map routine for the original Apple TV
 * COPYRIGHT:   Copyright 2023 DistroHopper39B (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>
#include <Uefi.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

#define NEXT_MEMORY_DESCRIPTOR(Descriptor, DescriptorSize) \
    (EFI_MEMORY_DESCRIPTOR*)((char*)(Descriptor) + (DescriptorSize))

/* GLOBALS *******************************************************************/

ULONG
AddMemoryDescriptor(
    IN OUT PFREELDR_MEMORY_DESCRIPTOR List,
    IN ULONG MaxCount,
    IN PFN_NUMBER BasePage,
    IN PFN_NUMBER PageCount,
    IN TYPE_OF_MEMORY MemoryType);

// we love statically allocated memory, don't we? :D
BIOS_MEMORY_MAP BiosMap[MAX_BIOS_DESCRIPTORS];

INT FreeldrDescCount;
FREELDR_MEMORY_DESCRIPTOR FreeldrMemMap[MAX_BIOS_DESCRIPTORS + 1];

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
                    Size,
                    Usage);
            }
        }
    }

    /* Add the memory descriptor */
    FreeldrDescCount = AddMemoryDescriptor(MemoryMap,
                                        MAX_BIOS_DESCRIPTORS,
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
                                        MAX_BIOS_DESCRIPTORS,
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
    ReserveMemory(MemoryMap, 0x1000, STACKLOW - 0x1000, LoaderFirmwareTemporary, "BIOS area");
    ReserveMemory(MemoryMap, STACKLOW, STACKADDR - STACKLOW, LoaderOsloaderStack, "FreeLdr stack");
    ReserveMemory(MemoryMap, FREELDR_BASE, FrLdrImageSize, LoaderLoadedProgram, "FreeLdr image");

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
BIOS_MEMORY_TYPE
UefiConvertToBiosType(EFI_MEMORY_TYPE MemoryType)
{
    switch (MemoryType)
    {
        // Unusable memory types
        case EfiReservedMemoryType:
        case EfiUnusableMemory:
        case EfiMemoryMappedIO:
        case EfiMemoryMappedIOPortSpace:
        case EfiPalCode:
            return BiosMemoryReserved;
        // Types usable after ACPI initialization
        case EfiACPIReclaimMemory:
            return BiosMemoryAcpiReclaim;
        // Usable memory types
        case EfiBootServicesCode:
        case EfiBootServicesData:
        case EfiConventionalMemory:
        case EfiRuntimeServicesCode:
        case EfiRuntimeServicesData:
        case EfiLoaderCode:
        case EfiLoaderData:
            return BiosMemoryUsable;
        // NVS memory
        case EfiACPIMemoryNVS:
            return BiosMemoryAcpiNvs;
        default:
            ERR("Unknown type. Memory map probably corrupted!\n");
            return BiosMemoryUnusable;
    }
}

static
TYPE_OF_MEMORY
BiosConvertToFreeldrType(BIOS_MEMORY_TYPE MemoryType)
{
    switch (MemoryType)
    {
        case BiosMemoryUsable:
            return LoaderFree;
        case BiosMemoryReserved:
            return LoaderFirmwarePermanent;
        case BiosMemoryAcpiReclaim:
            return LoaderFirmwareTemporary;
        case BiosMemoryAcpiNvs:
            return LoaderFirmwarePermanent;
        case BiosMemoryUnusable:
            return LoaderFirmwarePermanent;
        default:
            ERR("Unknown type %d. Memory map definitely corrupted!\n", MemoryType);
            return LoaderFirmwarePermanent;
    }
}

static
VOID
BiosAddMemoryRegion(PBIOS_MEMORY_MAP MemoryMap,
                    UINT32 *BiosNumberOfEntries,
                    UINT64 Start,
                    UINT64 Size,
                    BIOS_MEMORY_TYPE Type)
{
    UINT32 x = *BiosNumberOfEntries;
    if (x == MAX_BIOS_DESCRIPTORS)
    {
        ERR("Too many entries!\n");
        FrLdrBugCheckWithMessage(
            MEMORY_INIT_FAILURE,
            __FILE__,
            __LINE__,
            "Cannot create more than 80 BIOS memory descriptors!");
    }
    // Add on to existing entry if we can
    if ((x > 0)
        && (MemoryMap[x - 1].BaseAddress + MemoryMap[x - 1].Length == Start)
        && (MemoryMap[x - 1].Type == Type))
    {
        MemoryMap[x - 1].Length += Size;
    }
    else
    {
        MemoryMap[x].BaseAddress    = Start;
        MemoryMap[x].Length         = Size;
        MemoryMap[x].Type           = Type;
        (*BiosNumberOfEntries)++;
    }
}

static
PBIOS_MEMORY_MAP
UefiConvertToBiosMemoryMap(EFI_MEMORY_DESCRIPTOR *EfiMemoryMap,
                            SIZE_T MemoryMapSize,
                            SIZE_T MemoryDescriptorSize,
                            UINT32 *BiosNumberOfEntries)
{
    // we love statically allocated memory, don't we?
    BIOS_MEMORY_TYPE    BiosMemoryType;
    UINTN               i;
    UINTN               EfiNumberOfEntries = 0;
    
    EfiNumberOfEntries = (MemoryMapSize / MemoryDescriptorSize);
    
    for (i = 0; i < EfiNumberOfEntries; i++)
    {
        BiosMemoryType = UefiConvertToBiosType(EfiMemoryMap->Type);
        BiosAddMemoryRegion(BiosMap,
                            BiosNumberOfEntries,
                            EfiMemoryMap->PhysicalStart,
                            (EfiMemoryMap->NumberOfPages << EFI_PAGE_SHIFT),
                            BiosMemoryType);
        EfiMemoryMap = NEXT_MEMORY_DESCRIPTOR(EfiMemoryMap, MemoryDescriptorSize);
    }
        
    return BiosMap;
}

static
VOID
BiosConvertToFreeldrMap(PBIOS_MEMORY_MAP BiosMap,
                        UINT32 BiosMapNumberOfEntries)
{
    INT             i;
    TYPE_OF_MEMORY  MemoryType;
    
    for (i = 0; i < BiosMapNumberOfEntries; i++)
    {
        MemoryType = BiosConvertToFreeldrType(BiosMap[i].Type);
        SetMemory(FreeldrMemMap,
                BiosMap[i].BaseAddress,
                BiosMap[i].Length, MemoryType);
    }
}

PFREELDR_MEMORY_DESCRIPTOR
AppleTVMemGetMemoryMap(ULONG *MemoryMapSize)
{
    EFI_MEMORY_DESCRIPTOR   *EfiMemoryMap;
    SIZE_T                  EfiMemoryMapSize;
    SIZE_T                  EfiMemoryDescriptorSize;
    
    PBIOS_MEMORY_MAP        BiosMap;
    UINT32                  BiosMapNumberOfEntries = 0;
    
    // Convert EFI memory map to BIOS memory map.
    EfiMemoryMap            = (EFI_MEMORY_DESCRIPTOR *) BootArgs->EfiMemoryMap;
    EfiMemoryMapSize        = BootArgs->EfiMemoryMapSize;
    EfiMemoryDescriptorSize = BootArgs->EfiMemoryDescriptorSize;
    
    BiosMap = UefiConvertToBiosMemoryMap(EfiMemoryMap,
                                        EfiMemoryMapSize,
                                        EfiMemoryDescriptorSize,
                                        &BiosMapNumberOfEntries);
    // Convert BIOS memory map to FreeLoader memory map
    BiosConvertToFreeldrMap(BiosMap,
                            BiosMapNumberOfEntries);
    
    // reserve some ranges to prevent windows bugs
    SetMemory(FreeldrMemMap, 0x000000, 0x01000, LoaderFirmwarePermanent); // Realmode IVT / BDA
    SetMemory(FreeldrMemMap, 0x0A0000, 0x50000, LoaderFirmwarePermanent); // Video memory
    SetMemory(FreeldrMemMap, 0x0F0000, 0x10000, LoaderSpecialMemory); // ROM
    SetMemory(FreeldrMemMap, 0xFFF000, 0x01000, LoaderSpecialMemory); // unusable memory (do we really need this?)
    
    *MemoryMapSize = PcMemFinalizeMemoryMap(FreeldrMemMap);
    
    return FreeldrMemMap;
}