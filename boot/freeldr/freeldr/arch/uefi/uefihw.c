/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware detection routines
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <uefildr.h>
#include <drivers/bootvid/framebuf.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

/* GLOBALS *******************************************************************/

extern EFI_SYSTEM_TABLE * GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
extern UCHAR PcBiosDiskCount;
extern EFI_MEMORY_DESCRIPTOR* EfiMemoryMap;
extern UINT32 FreeldrDescCount;

BOOLEAN AcpiPresent = FALSE;

/* Maximum number of COM and LPT ports */
#define MAX_COM_PORTS   4
#define MAX_LPT_PORTS   3

/* No Mouse */
#define MOUSE_TYPE_NONE            0
/* Microsoft Mouse with 2 buttons */
#define MOUSE_TYPE_MICROSOFT       1
/* Logitech Mouse with 3 buttons */
#define MOUSE_TYPE_LOGITECH        2
/* Microsoft Wheel Mouse (aka Z Mouse) */
#define MOUSE_TYPE_WHEELZ          3
/* Mouse Systems Mouse */
#define MOUSE_TYPE_MOUSESYSTEMS    4


/* Timeout in ms for sending to keyboard controller. */
#define CONTROLLER_TIMEOUT                              250

/* PS2 stuff */

/* Controller registers. */
#define CONTROLLER_REGISTER_STATUS                      0x64
#define CONTROLLER_REGISTER_CONTROL                     0x64
#define CONTROLLER_REGISTER_DATA                        0x60

/* Controller commands. */
#define CONTROLLER_COMMAND_READ_MODE                    0x20
#define CONTROLLER_COMMAND_WRITE_MODE                   0x60
#define CONTROLLER_COMMAND_GET_VERSION                  0xA1
#define CONTROLLER_COMMAND_MOUSE_DISABLE                0xA7
#define CONTROLLER_COMMAND_MOUSE_ENABLE                 0xA8
#define CONTROLLER_COMMAND_TEST_MOUSE                   0xA9
#define CONTROLLER_COMMAND_SELF_TEST                    0xAA
#define CONTROLLER_COMMAND_KEYBOARD_TEST                0xAB
#define CONTROLLER_COMMAND_KEYBOARD_DISABLE             0xAD
#define CONTROLLER_COMMAND_KEYBOARD_ENABLE              0xAE
#define CONTROLLER_COMMAND_WRITE_MOUSE_OUTPUT_BUFFER    0xD3
#define CONTROLLER_COMMAND_WRITE_MOUSE                  0xD4

/* Controller status */
#define CONTROLLER_STATUS_OUTPUT_BUFFER_FULL            0x01
#define CONTROLLER_STATUS_INPUT_BUFFER_FULL             0x02
#define CONTROLLER_STATUS_SELF_TEST                     0x04
#define CONTROLLER_STATUS_COMMAND                       0x08
#define CONTROLLER_STATUS_UNLOCKED                      0x10
#define CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL      0x20
#define CONTROLLER_STATUS_GENERAL_TIMEOUT               0x40
#define CONTROLLER_STATUS_PARITY_ERROR                  0x80
#define AUX_STATUS_OUTPUT_BUFFER_FULL                   (CONTROLLER_STATUS_OUTPUT_BUFFER_FULL | \
                                                         CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL)

/* FUNCTIONS *****************************************************************/

BOOLEAN IsAcpiPresent(VOID)
{
    return AcpiPresent;
}

static
PRSDP_DESCRIPTOR
FindAcpiBios(VOID)
{
    UINTN i;
    RSDP_DESCRIPTOR* rsdp = NULL;
    EFI_GUID acpi2_guid = EFI_ACPI_20_TABLE_GUID;

    for (i = 0; i < GlobalSystemTable->NumberOfTableEntries; i++)
    {
        if (!memcmp(&GlobalSystemTable->ConfigurationTable[i].VendorGuid,
                    &acpi2_guid, sizeof(acpi2_guid)))
        {
            rsdp = (RSDP_DESCRIPTOR*)GlobalSystemTable->ConfigurationTable[i].VendorTable;
            break;
        }
    }

    return rsdp;
}

VOID
DetectAcpiBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PRSDP_DESCRIPTOR Rsdp;
    PACPI_BIOS_DATA AcpiBiosData;
    ULONG TableSize;

    Rsdp = FindAcpiBios();

    if (Rsdp)
    {
        /* Set up the flag in the loader block */
        AcpiPresent = TRUE;

        /* Calculate the table size */
        TableSize = FreeldrDescCount * sizeof(BIOS_MEMORY_MAP) +
            sizeof(ACPI_BIOS_DATA) - sizeof(BIOS_MEMORY_MAP);

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

        AcpiBiosData->Count = FreeldrDescCount;
        memcpy(AcpiBiosData->MemoryMap, EfiMemoryMap,
            FreeldrDescCount * sizeof(BIOS_MEMORY_MAP));

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
}

extern REACTOS_INTERNAL_BGCONTEXT framebufferData;
/****/extern EFI_PIXEL_BITMASK UefiGopPixelBitmask;/****/

static VOID
DetectDisplayController(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_FRAMEBUF_DEVICE_DATA FramebufferData;
    ULONG Size;

    if (framebufferData.BufferSize == 0)
        return;

    ERR("\nStructure sizes:\n"
        "    sizeof(CM_PARTIAL_RESOURCE_LIST)       = %lu\n"
        "    sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) = %lu\n"
        "    sizeof(CM_FRAMEBUF_DEVICE_DATA)        = %lu\n\n",
        sizeof(CM_PARTIAL_RESOURCE_LIST),
        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR),
        sizeof(CM_FRAMEBUF_DEVICE_DATA));

    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
           sizeof(CM_FRAMEBUF_DEVICE_DATA);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version  = ARC_VERSION;
    PartialResourceList->Revision = ARC_REVISION;
    PartialResourceList->Count = 2;

    /* Set Memory */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypeMemory;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
    PartialDescriptor->u.Memory.Start.QuadPart = framebufferData.BaseAddress;
    PartialDescriptor->u.Memory.Length = framebufferData.BufferSize;

    /* Set framebuffer-specific data */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
    PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = 0;
    PartialDescriptor->u.DeviceSpecificData.DataSize =
        sizeof(CM_FRAMEBUF_DEVICE_DATA);

    /* Get pointer to framebuffer-specific data */
    FramebufferData = (PVOID)(PartialDescriptor + 1);
    RtlZeroMemory(FramebufferData, sizeof(*FramebufferData));
    FramebufferData->Version  = 2;
    FramebufferData->Revision = 0;

    FramebufferData->VideoClock = 0; // FIXME: Use EDID

    /* Horizontal and Vertical resolution in pixels */
    FramebufferData->ScreenWidth  = framebufferData.ScreenWidth;
    FramebufferData->ScreenHeight = framebufferData.ScreenHeight;

    /* Number of pixel elements per video memory line */
    FramebufferData->PixelsPerScanLine = framebufferData.PixelsPerScanLine;

    //
    // TODO: Investigate display rotation!
    //
    // See OpenCorePkg OcConsoleLib/ConsoleGop.c
    // if ((mGop.Rotation == 90) || (mGop.Rotation == 270))
    if (FramebufferData->ScreenWidth < FramebufferData->ScreenHeight)
    {
        #define SWAP(x, y) { (x) ^= (y); (y) ^= (x); (x) ^= (y); }
        SWAP(FramebufferData->ScreenWidth, FramebufferData->ScreenHeight);
        FramebufferData->PixelsPerScanLine = FramebufferData->ScreenWidth;
        #undef SWAP
    }

    /* Physical format of the pixel */
    // ASSERT(sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) == 4);
    switch (framebufferData.PixelFormat)
    {
        case PixelRedGreenBlueReserved8BitPerColor:
        {
            FramebufferData->BitsPerPixel = (8 * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
           *(EFI_PIXEL_BITMASK*)&FramebufferData->PixelInformation = EfiPixelMasks[framebufferData.PixelFormat];
            break;
        }

        case PixelBlueGreenRedReserved8BitPerColor:
        {
            FramebufferData->BitsPerPixel = (8 * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
            FramebufferData->PixelInformation.RedMask      = 0x00FF0000;
            FramebufferData->PixelInformation.GreenMask    = 0x0000FF00;
            FramebufferData->PixelInformation.BlueMask     = 0x000000FF;
            FramebufferData->PixelInformation.ReservedMask = 0xFF000000;
            break;
        }

        case PixelBitMask:
        {
            FramebufferData->BitsPerPixel =
                PixelBitmasksToBpp(UefiGopPixelBitmask.RedMask,
                                   UefiGopPixelBitmask.GreenMask,
                                   UefiGopPixelBitmask.BlueMask,
                                   UefiGopPixelBitmask.ReservedMask);
           *(EFI_PIXEL_BITMASK*)&FramebufferData->PixelInformation = UefiGopPixelBitmask;
           //FramebufferData->PixelInformation.RedMask      = UefiGopPixelBitmask.RedMask;
           //FramebufferData->PixelInformation.GreenMask    = UefiGopPixelBitmask.GreenMask;
           //FramebufferData->PixelInformation.BlueMask     = UefiGopPixelBitmask.BlueMask;
           //FramebufferData->PixelInformation.ReservedMask = UefiGopPixelBitmask.ReservedMask;
            break;
        }

        case PixelBltOnly:
        default:
        {
            ERR("Unsupported UFEI GOP format %lu\n", framebufferData.PixelFormat);
            FramebufferData->BitsPerPixel = 0;
            RtlZeroMemory(&FramebufferData->PixelInformation,
                          sizeof(FramebufferData->PixelInformation));
            break;
        }
    }

    FldrCreateComponentKey(BusKey,
                           ControllerClass,
                           DisplayController,
                           Output | ConsoleOut,
                           0,
                           0xFFFFFFFF,
                           "UEFI GOP Framebuffer",
                           PartialResourceList,
                           Size,
                           &ControllerKey);

    // NOTE: Don't add a MonitorPeripheral for now...
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
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) -
           sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version  = ARC_VERSION;
    PartialResourceList->Revision = ARC_REVISION;
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


PCONFIGURATION_COMPONENT_DATA
UefiHwDetect(
    _In_opt_ PCSTR Options)
{
    PCONFIGURATION_COMPONENT_DATA SystemKey;
    ULONG BusNumber = 0;

    TRACE("DetectHardware()\n");

    /* Create the 'System' key */
#if defined(_M_IX86) || defined(_M_AMD64)
    /* Create the 'System' key */
    FldrCreateSystemKey(&SystemKey, FALSE, "AT/AT COMPATIBLE");
#elif defined(_M_IA64)
    FldrCreateSystemKey(&SystemKey, "Intel Itanium processor family");
#elif defined(_M_ARM) || defined(_M_ARM64)
    FldrCreateSystemKey(&SystemKey, "ARM processor family");
#else
    #error Please define a system key for your architecture
#endif

    /* Detect ACPI */
    DetectAcpiBios(SystemKey, &BusNumber);
    DetectInternal(SystemKey, &BusNumber);

    TRACE("DetectHardware() Done\n");
    return SystemKey;
}
