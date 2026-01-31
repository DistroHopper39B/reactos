/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware detection routines for the original Apple TV
 * COPYRIGHT:   Copyright 2023 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>
#include "../../vidfb.h"

/* UEFI support */
#include <Uefi.h>
#include <Acpi.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

/* GLOBALS *******************************************************************/

BOOLEAN AcpiPresent = FALSE;

static unsigned int delay_count = 1;

PCHAR GetHarddiskIdentifier(UCHAR DriveNumber); /* hwdisk.c */
USHORT WinLdrDetectVersion(VOID); /* winldr.c */

#define MILLISEC     (10)
#define PRECISION    (8)

#define CLOCK_TICK_RATE 1193182

#define HZ (100)
#define LATCH (CLOCK_TICK_RATE / HZ)

#define SMBIOS_TABLE_GUID \
  { \
    0xeb9d2d31, 0x2d88, 0x11d3, {0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define SMBIOS_TABLE_LOW 0xF0000

extern ULONG_PTR VramAddress;
extern ULONG VramSize;
extern PCM_FRAMEBUF_DEVICE_DATA FrameBufferData;

/* FUNCTIONS *****************************************************************/

BOOLEAN IsAcpiPresent(VOID)
{
    return AcpiPresent;
}

static
VOID
__StallExecutionProcessor(ULONG Loops)
{
    register volatile unsigned int i;
    for (i = 0; i < Loops; i++);
}

VOID StallExecutionProcessor(ULONG Microseconds)
{
    ULONGLONG LoopCount = ((ULONGLONG)delay_count * (ULONGLONG)Microseconds) / 1000ULL;
    __StallExecutionProcessor((ULONG)LoopCount);
}

static
ULONG
Read8254Timer(VOID)
{
    ULONG Count;

    WRITE_PORT_UCHAR((PUCHAR)0x43, 0x00);
    Count = READ_PORT_UCHAR((PUCHAR)0x40);
    Count |= READ_PORT_UCHAR((PUCHAR)0x40) << 8;

    return Count;
}

static
VOID
WaitFor8254Wraparound(VOID)
{
    ULONG CurCount;
    ULONG PrevCount = ~0;
    LONG Delta;

    CurCount = Read8254Timer();

    do
    {
        PrevCount = CurCount;
        CurCount = Read8254Timer();
        Delta = CurCount - PrevCount;

        /*
         * This limit for delta seems arbitrary, but it isn't, it's
         * slightly above the level of error a buggy Mercury/Neptune
         * chipset timer can cause.
         */
    }
    while (Delta < 300);
}

static
PVOID
FindUefiVendorTable(EFI_SYSTEM_TABLE *SystemTable, EFI_GUID Guid)
{
    ULONG i;
    
    for (i = 0; i < SystemTable->NumberOfTableEntries; i++)
    {
        if (!memcmp(&SystemTable->ConfigurationTable[i].VendorGuid,
            &Guid, sizeof(EFI_GUID)))
        {
            return SystemTable->ConfigurationTable[i].VendorTable;
        }
    }
    
    return NULL;
}

VOID
HalpCalibrateStallExecution(VOID)
{
    ULONG i;
    ULONG calib_bit;
    ULONG CurCount;

    /* Initialise timer interrupt with MILLISECOND ms interval        */
    WRITE_PORT_UCHAR((PUCHAR)0x43, 0x34);  /* binary, mode 2, LSB/MSB, ch 0 */
    WRITE_PORT_UCHAR((PUCHAR)0x40, LATCH & 0xff); /* LSB */
    WRITE_PORT_UCHAR((PUCHAR)0x40, LATCH >> 8); /* MSB */

    /* Stage 1:  Coarse calibration                                   */

    delay_count = 1;

    do
    {
        /* Next delay count to try */
        delay_count <<= 1;

        WaitFor8254Wraparound();

        /* Do the delay */
        __StallExecutionProcessor(delay_count);

        CurCount = Read8254Timer();
    }
    while (CurCount > LATCH / 2);

    /* Get bottom value for delay */
    delay_count >>= 1;

    /* Stage 2:  Fine calibration                                     */

    /* Which bit are we going to test */
    calib_bit = delay_count;

    for (i = 0; i < PRECISION; i++)
    {
        /* Next bit to calibrate */
        calib_bit >>= 1;

        /* If we have done all bits, stop */
        if (!calib_bit) break;

        /* Set the bit in delay_count */
        delay_count |= calib_bit;

        WaitFor8254Wraparound();

        /* Do the delay */
        __StallExecutionProcessor(delay_count);

        CurCount = Read8254Timer();
        /* If a tick has passed, turn the calibrated bit back off */
        if (CurCount <= LATCH / 2)
            delay_count &= ~calib_bit;
    }

    /* We're finished:  Do the finishing touches */

    /* Calculate delay_count for 1ms */
    delay_count /= (MILLISEC / 2);
}

static
BOOLEAN
AppleTVFindPciBios(PPCI_REGISTRY_INFO BusData)
{
    /* We hardcode PCI BIOS here */
    
    BusData->MajorRevision = 0x02;
    BusData->MinorRevision = 0x10;
    BusData->NoBuses = 7;
    BusData->HardwareMechanism = 1;
    return TRUE;
}

VOID
DetectPciBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCI_REGISTRY_INFO BusData;
    ULONG Size;
    PCONFIGURATION_COMPONENT_DATA BusKey;
    ULONG i;

    AppleTVFindPciBios(&BusData);
    
    /* Set 'Configuration Data' value */
    Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);

    /* Increment bus number */
    (*BusNumber)++;

    // DetectPciIrqRoutingTable(BiosKey);

    /* Report PCI buses */
    for (i = 0; i < (ULONG)BusData.NoBuses; i++)
    {
        /* Check if this is the first bus */
        if (i == 0)
        {
            /* Set 'Configuration Data' value */
            Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                PartialDescriptors) +
                   sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
                   sizeof(PCI_REGISTRY_INFO);
            PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
            if (!PartialResourceList)
            {
                ERR("Failed to allocate resource descriptor! Ignoring remaining PCI buses. (i = %lu, NoBuses = %lu)\n",
                    i, (ULONG)BusData.NoBuses);
                return;
            }

            /* Initialize resource descriptor */
            RtlZeroMemory(PartialResourceList, Size);
            PartialResourceList->Version = 1;
            PartialResourceList->Revision = 1;
            PartialResourceList->Count = 1;
            PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
            PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
            PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
            PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(PCI_REGISTRY_INFO);
            memcpy(&PartialResourceList->PartialDescriptors[1],
                   &BusData,
                   sizeof(PCI_REGISTRY_INFO));
        }
        else
        {
            /* Set 'Configuration Data' value */
            Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                PartialDescriptors);
            PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
            if (!PartialResourceList)
            {
                ERR("Failed to allocate resource descriptor! Ignoring remaining PCI buses. (i = %lu, NoBuses = %lu)\n",
                    i, (ULONG)BusData.NoBuses);
                return;
            }

            /* Initialize resource descriptor */
            RtlZeroMemory(PartialResourceList, Size);
        }

        /* Create the bus key */
        FldrCreateComponentKey(SystemKey,
                               AdapterClass,
                               MultiFunctionAdapter,
                               0x0,
                               0x0,
                               0xFFFFFFFF,
                               "PCI",
                               PartialResourceList,
                               Size,
                               &BusKey);

        /* Increment bus number */
        (*BusNumber)++;
    }

}

VOID
DetectAcpiBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCONFIGURATION_COMPONENT_DATA   BiosKey;
    PCM_PARTIAL_RESOURCE_LIST       PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PRSDP_DESCRIPTOR                Rsdp;
    PACPI_BIOS_DATA                 AcpiBiosData;
    ULONG                           TableSize;
    EFI_SYSTEM_TABLE                *SystemTable;
    EFI_GUID                        Guid;
    
    SystemTable = (EFI_SYSTEM_TABLE *) BootArgs->EfiSystemTable;
    
    // Detect what version of NT we're running
    // Note: This information should probably be passed into HwDetect
#if 1
    USHORT WindowsVersion = WinLdrDetectVersion();
    ASSERT(WindowsVersion != 0);

    if (WindowsVersion >= _WIN32_WINNT_WINXP)
    {
        // Windows XP and later: Use ACPI 2.0 table.
        Guid = (EFI_GUID) EFI_ACPI_20_TABLE_GUID;
    }
    else
    {
        // Windows 2000 and earlier: Use ACPI 1.0 table.
        // Note: This breaks software reboot on the Apple TV and may be completely broken on newer
        // devices.
        Guid = (EFI_GUID) ACPI_10_TABLE_GUID;
    }
#else
    Guid = (EFI_GUID) ACPI_10_TABLE_GUID;
#endif
    
    Rsdp = FindUefiVendorTable(SystemTable, Guid);

    if (Rsdp)
    {
        /* Set up the flag in the loader block */
        AcpiPresent = TRUE;

        /* Calculate the table size */
        TableSize = sizeof(ACPI_BIOS_DATA);

        /* Set 'Configuration Data' value */
        PartialResourceList = FrLdrHeapAlloc(sizeof(CM_PARTIAL_RESOURCE_LIST) +
                                             TableSize, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor\n");
            return;
        }

        RtlZeroMemory(PartialResourceList, sizeof(CM_PARTIAL_RESOURCE_LIST) + TableSize);
        PartialResourceList->Version = 0;
        PartialResourceList->Revision = 0;
        PartialResourceList->Count = 1;

        PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
        PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->u.DeviceSpecificData.DataSize = TableSize;

        /* Fill the table */
        AcpiBiosData = (PACPI_BIOS_DATA)&PartialResourceList->PartialDescriptors[1];

        if (Rsdp->revision > 0)
        {
            TRACE("ACPI >1.0, using XSDT address\n");
            AcpiBiosData->RSDTAddress.QuadPart = Rsdp->xsdt_physical_address;
        }
        else
        {
            TRACE("ACPI 1.0, using RSDT address\n");
            AcpiBiosData->RSDTAddress.LowPart = Rsdp->rsdt_physical_address;
        }

        TRACE("RSDT %p, data size %x\n", Rsdp->rsdt_physical_address, TableSize);

        /* Create new bus key */
        FldrCreateComponentKey(SystemKey,
                               AdapterClass,
                               MultiFunctionAdapter,
                               0x0,
                               0x0,
                               0xFFFFFFFF,
                               "ACPI BIOS",
                               PartialResourceList,
                               sizeof(CM_PARTIAL_RESOURCE_LIST) + TableSize,
                               &BiosKey);

        /* Increment bus number */
        (*BusNumber)++;
    }
    else
    {
        // NT will not boot without ACPI unless a PIRQ table is present.
        // EFI platforms like the Apple TV should never reach this.
        UiMessageBoxCritical("Cannot find ACPI tables!");
    }
}

static VOID
DetectDisplayController(
    _In_ PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_FRAMEBUF_DEVICE_DATA FramebufData;
    ULONG Size;

    if (!VramAddress || (VramSize == 0) || !FrameBufferData)
        return;

    Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors[2]) + sizeof(*FramebufData);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version  = 1;
    PartialResourceList->Revision = 2;
    PartialResourceList->Count = 2;

    /* Set Memory */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypeMemory;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
    PartialDescriptor->u.Memory.Start.QuadPart = VramAddress;
    PartialDescriptor->u.Memory.Length = VramSize;

    /* Set framebuffer-specific data */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
    PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = 0;
    PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(*FramebufData);

    /* Get pointer to framebuffer-specific data */
    FramebufData = (PCM_FRAMEBUF_DEVICE_DATA)(PartialDescriptor + 1);
    RtlCopyMemory(FramebufData, FrameBufferData, sizeof(*FrameBufferData));
    FramebufData->Version  = 1;
    FramebufData->Revision = 3;
    FramebufData->VideoClock = 0; // FIXME: Use EDID

    FldrCreateComponentKey(BusKey,
                           ControllerClass,
                           DisplayController,
                           Output | ConsoleOut,
                           0,
                           0xFFFFFFFF,
                           "Apple TV Framebuffer",
                           PartialResourceList,
                           Size,
                           &ControllerKey);

    // NOTE: Don't add a MonitorPeripheral for now.
    // We should use EDID data for it.
}

static
VOID
DetectInternal(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCONFIGURATION_COMPONENT_DATA BusKey;
    ULONG Size;

    /* Set 'Configuration Data' value */
    Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version  = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 0;

    /* Create new bus key */
    FldrCreateComponentKey(SystemKey,
                           AdapterClass,
                           MultiFunctionAdapter,
                           0,
                           0,
                           0xFFFFFFFF,
                           "UEFI Internal",
                           PartialResourceList,
                           Size,
                           &BusKey);

    /* Increment bus number */
    (*BusNumber)++;

    /* Detect devices that do not belong to "standard" buses */
    DetectDisplayController(BusKey);

    /* FIXME: Detect more devices */
}

static
VOID
DetectSmBios(VOID)
{
    PSMBIOS_TABLE_HEADER    SmBiosTable;
    EFI_SYSTEM_TABLE        *SystemTable;
    
    SystemTable = (EFI_SYSTEM_TABLE *) BootArgs->EfiSystemTable;
    SmBiosTable = FindUefiVendorTable(SystemTable, (EFI_GUID) SMBIOS_TABLE_GUID);
    if (!SmBiosTable)
    {
        // This should never happen, but should not result in a critical system error if it does.
        ERR("No SMBIOS table found!\n");
        return;
    }
    
    // Copy SMBIOS table to low memory.
    // Note: On most hardware, low memory is read-only and must be unlocked using either the
    // EFI Legacy Region Protocol or PAM/MTRR; see UefiSeven/CSMWrap.
    // The Apple TV is a notable exception.
    RtlCopyMemory((PVOID) SMBIOS_TABLE_LOW, SmBiosTable, sizeof(SMBIOS_TABLE_HEADER));
}


PCONFIGURATION_COMPONENT_DATA
AppleTVHwDetect(_In_opt_ PCSTR Options)
{
    PCONFIGURATION_COMPONENT_DATA SystemKey;
    ULONG BusNumber = 0;

    TRACE("MachHwDetect()\n");
    
    /* Create the 'System' key */
    FldrCreateSystemKey(&SystemKey, "Apple TV (1st generation)");

    DetectPciBios(SystemKey, &BusNumber);
    DetectAcpiBios(SystemKey, &BusNumber);
    DetectInternal(SystemKey, &BusNumber);
    DetectSmBios();

    TRACE("MachHwDetect() Done\n");
    return SystemKey;
}

VOID
FrLdrCheckCpuCompatibility(VOID)
{
    INT CpuInformation[4] = {-1};
    ULONG NumberOfIds;

    /* Check if the processor first supports ID 1 */
    __cpuid(CpuInformation, 0);

    NumberOfIds = CpuInformation[0];

    if (NumberOfIds == 0)
    {
        FrLdrBugCheckWithMessage(MISSING_HARDWARE_REQUIREMENTS,
                                 __FILE__,
                                 __LINE__,
                                 "ReactOS requires the CPUID instruction to return "
                                 "more than one supported ID.\n\n");
    }

    /* NumberOfIds will be greater than 1 if the processor is new enough */
    if (NumberOfIds == 1)
    {
        INT ProcessorFamily;

        /* Get information */
        __cpuid(CpuInformation, 1);

        ProcessorFamily = (CpuInformation[0] >> 8) & 0xF;

        /* If it's Family 4 or lower, bugcheck */
        if (ProcessorFamily < 5)
        {
            FrLdrBugCheckWithMessage(MISSING_HARDWARE_REQUIREMENTS,
                                     __FILE__,
                                     __LINE__,
                                     "Processor is too old (family %u < 5)\n"
                                     "ReactOS requires a Pentium-level processor or newer.",
                                     ProcessorFamily);
        }
    }
}
