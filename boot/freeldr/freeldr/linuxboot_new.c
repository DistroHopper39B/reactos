/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Linux boot support for FreeLoader
 * COPYRIGHT:   Copyright 1999-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2026 Sylas Hollander <distrohopper39b.business@gmail.com>
 */
 
 /* 
  * The Linux/x86 boot protocol is explained at: 
  * https://www.kernel.org/doc/html/latest/arch/x86/boot.html
  */

#if defined(_M_IX86) || defined(_M_AMD64)

/* INCLUDES *******************************************************************/
 
#include <freeldr.h>

#ifdef UEFIBOOT
#include <Uefi.h>
#include "arch/vidfb.h"
#endif

#include <debug.h>
DBG_DEFAULT_CHANNEL(LINUX);

/* GLOBALS ********************************************************************/

#define LINUX_READ_CHUNK_SIZE   0x20000 // Read 128k at a time

struct boot_params  *BootParams = NULL; 
PVOID               LinuxKernelLoadAddress = NULL;
PVOID               LinuxInitrdLoadAddress = NULL;
ULONG               TotalSize = 0;
ULONG               TotalLoadPercentage = 0;

PRSDP_DESCRIPTOR FindAcpiBios(VOID);

static BOOLEAN ValidateLinuxKernel(ULONG LinuxKernelFile);
static PVOID ReadLinuxKernel(ULONG File, ULONG Size, ULONG Offset, PCSTR Name);
static PVOID ReadLinuxInitrd(ULONG File, ULONG Size, ULONG KernelSize, ULONG MaxAddr, PCSTR Name);

#ifdef UEFIBOOT
extern ULONG_PTR VramAddress;
extern ULONG VramSize;
extern PCM_FRAMEBUF_DEVICE_DATA FrameBufferData;

extern EFI_SYSTEM_TABLE *GlobalSystemTable;
extern EFI_MEMORY_DESCRIPTOR* EfiMemoryMap;
extern UINTN MapSize;
extern UINTN DescriptorSize;
extern UINT32 DescriptorVersion;

#define NEXT_MEMORY_DESCRIPTOR(Descriptor, DescriptorSize) \
    (EFI_MEMORY_DESCRIPTOR*)((char*)(Descriptor) + (DescriptorSize))

#else
extern BIOS_MEMORY_MAP PcBiosMemoryMap[];
extern ULONG PcBiosMapCount;
#endif

/* FUNCTIONS ******************************************************************/

static VOID
RemoveQuotes(
    IN OUT PSTR QuotedString)
{
    PCHAR  p;
    PSTR   Start;
    SIZE_T Size;

    /* Skip spaces up to " */
    p = QuotedString;
    while (*p == ' ' || *p == '\t' || *p == '"')
        ++p;
    Start = p;

    /* Go up to next " */
    while (*p != ANSI_NULL && *p != '"')
        ++p;
    /* NULL-terminate */
    *p = ANSI_NULL;

    /* Move the NULL-terminated string back into 'QuotedString' in place */
    Size = (strlen(Start) + 1) * sizeof(CHAR);
    memmove(QuotedString, Start, Size);
}

// Based on limine gop.c - convert linear masks to mask shift
#ifdef UEFIBOOT
static VOID
LinearMaskToMaskShift(UINT8 *Mask, UINT8 *Shift, UINT32 LinearMask)
{
    *Shift = 0;
    *Mask = 0;
    
    if (LinearMask == 0)
        return;
    
    while ((LinearMask & 1) == 0)
    {
        (*Shift)++;
        LinearMask >>= 1;
    }
    
    while ((LinearMask & 1) == 1)
    {
        (*Mask)++;
        LinearMask >>= 1;
    }
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
        case EfiRuntimeServicesCode:
        case EfiRuntimeServicesData:
            return BiosMemoryReserved;
        // Types usable after ACPI initialization
        case EfiACPIReclaimMemory:
            return BiosMemoryAcpiReclaim;
        // Usable memory types
        case EfiBootServicesCode:
        case EfiBootServicesData:
        case EfiConventionalMemory:
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

static VOID
BiosAddMemoryRegion(struct boot_e820_entry *MemoryMap,
                    UINT8 *BiosNumberOfEntries,
                    UINT64 Start,
                    UINT64 Size,
                    BIOS_MEMORY_TYPE Type)
{
    UINT8 Entry = *BiosNumberOfEntries;
    if (Entry == MAX_BIOS_DESCRIPTORS)
    {
        ERR("Too many entries!\n");
        FrLdrBugCheckWithMessage(
            MEMORY_INIT_FAILURE,
            __FILE__,
            __LINE__,
            "Cannot create more than 80 BIOS memory descriptors!");
    }
    // Add on to existing entry if we can
    if ((Entry > 0)
        && (MemoryMap[Entry - 1].addr + MemoryMap[Entry - 1].size == Start)
        && (MemoryMap[Entry - 1].type == Type))
    {
        MemoryMap[Entry - 1].size += Size;
    }
    else
    {
        MemoryMap[Entry].addr       = Start;
        MemoryMap[Entry].size       = Size;
        MemoryMap[Entry].type       = Type;
        (*BiosNumberOfEntries)++;
    }
}

static VOID
LinuxFillMemoryMap(struct boot_e820_entry *MemoryMap, UINT8 *NumberOfEntries)
{
    ULONG                   i;    
    ULONG                   EfiNumberOfEntries = (MapSize / DescriptorSize);
    EFI_MEMORY_DESCRIPTOR   *CurrentDescriptor = EfiMemoryMap;
    
    
    for (i = 0; i < EfiNumberOfEntries; i++)
    {
        BiosAddMemoryRegion(MemoryMap, NumberOfEntries,
                        CurrentDescriptor->PhysicalStart,
                        EFI_PAGES_TO_SIZE(CurrentDescriptor->NumberOfPages),
                        UefiConvertToBiosType(CurrentDescriptor->Type));
        CurrentDescriptor = NEXT_MEMORY_DESCRIPTOR(CurrentDescriptor, DescriptorSize);
    }
}

#else
static VOID
LinuxFillMemoryMap(struct boot_e820_entry *MemoryMap, UINT8 *NumberOfEntries)
{
    ULONG i;
    for (i = 0; i < PcBiosMapCount; i++)
    {
        MemoryMap[i].addr   = PcBiosMemoryMap[i].BaseAddress;
        MemoryMap[i].size   = PcBiosMemoryMap[i].Length;
        MemoryMap[i].type   = PcBiosMemoryMap[i].Type;
    }
    
    *NumberOfEntries = PcBiosMapCount;
}
#endif

ARC_STATUS
LoadAndBootLinux(
    _In_ ULONG Argc,
    _In_ PCHAR Argv[],
    _In_ PCHAR Envp[])
{
    ARC_STATUS          Status;
    
    PCSTR               Description;
    PCSTR               ArgValue;
    PCSTR               BootPath;
    PCSTR               LinuxKernelName = NULL;
    PCSTR               LinuxInitrdName = NULL;
    CHAR                ArcPath[MAX_PATH];
    CHAR                LinuxBootDescription[80];
    
    ULONG               LinuxKernelFile = 0;
    ULONG               LinuxInitrdFile = 0;
    ULONG               LinuxKernelSize = 0;
    ULONG               LinuxInitrdSize = 0;
    
    FILEINFORMATION     FileInfo;
    LARGE_INTEGER       Position;
    struct setup_header *SetupHeader = NULL;
    ULONG               SetupHeaderEnd = 0;
    ULONG               SetupCodeSize = 0;
    ULONG               LinuxKernelOffset = 0;
    
    PSTR                LinuxCommandLine = NULL;
    
    struct screen_info  *ScreenInfo;

    Description = GetArgumentValue(Argc, Argv, "BootType");
    if (Description && *Description)
        RtlStringCbPrintfA(LinuxBootDescription,
                    sizeof(LinuxBootDescription),
                    "Loading %s...",
                    Description);
    else
        strcpy(LinuxBootDescription, "Loading Linux...");
    
    UiDrawBackdrop(UiGetScreenHeight());
    UiDrawStatusText(LinuxBootDescription);
    UiDrawProgressBarCenter(LinuxBootDescription);
    
    /* Find all the message box settings and run them */
    UiShowMessageBoxesInArgv(Argc, Argv);
    
    /*
     * Check whether we have a "BootPath" value (takes precedence
     * over both "BootDrive" and "BootPartition").
     */
    BootPath = GetArgumentValue(Argc, Argv, "BootPath");
    if (!BootPath || !*BootPath)
    {
        /* We don't have one, check whether we use "BootDrive" and "BootPartition" */

        /* Retrieve the boot drive (optional, fall back to using default path otherwise) */
        ArgValue = GetArgumentValue(Argc, Argv, "BootDrive");
        if (ArgValue && *ArgValue)
        {
            UCHAR DriveNumber = DriveMapGetBiosDriveNumber(ArgValue);

            /* Retrieve the boot partition (not optional and cannot be zero) */
            ULONG PartitionNumber = 0;
            ArgValue = GetArgumentValue(Argc, Argv, "BootPartition");
            if (ArgValue && *ArgValue)
                PartitionNumber = atoi(ArgValue);
            if (PartitionNumber == 0)
            {
                UiMessageBox("Boot partition cannot be 0!");
                goto LinuxBootFailed;
                // return EINVAL;
            }

            /* Construct the corresponding ARC path */
            ConstructArcPath(ArcPath, "", DriveNumber, PartitionNumber);
            *strrchr(ArcPath, '\\') = ANSI_NULL; // Trim the trailing path separator.

            BootPath = ArcPath;
        }
        else
        {
            /* Fall back to using the system partition as default path */
            BootPath = GetArgumentValue(Argc, Argv, "SystemPartition");
        }
    }

    /* Get the kernel name */
    LinuxKernelName = GetArgumentValue(Argc, Argv, "Kernel");
    if (!LinuxKernelName || !*LinuxKernelName)
    {
        UiMessageBox("Linux kernel filename not specified for selected OS!");
        goto LinuxBootFailed;
    }
    
    /* Get the initrd name (optional) */
    LinuxInitrdName = GetArgumentValue(Argc, Argv, "Initrd");

    /* Get the command line (optional) */
    LinuxCommandLine = GetArgumentValue(Argc, Argv, "CommandLine");
    if (LinuxCommandLine && *LinuxCommandLine)
    {
        RemoveQuotes(LinuxCommandLine);
    }
    
    /* Open the kernel */
    Status = FsOpenFile(LinuxKernelName,
                    BootPath,
                    OpenReadOnly,
                    &LinuxKernelFile);
    if (Status != ESUCCESS)
    {
        UiMessageBox("Linux kernel '%s' not found.", LinuxKernelName);
        goto LinuxBootFailed;
    }

    /* Open the initrd file image (if necessary) */
    if (LinuxInitrdName)
    {
        Status = FsOpenFile(LinuxInitrdName,
                        BootPath,
                        OpenReadOnly,
                        &LinuxInitrdFile);
        if (Status != ESUCCESS)
        {
            UiMessageBox("Linux initrd image '%s' not found.", LinuxInitrdName);
            goto LinuxBootFailed;
        }
    }
    
    if (!ValidateLinuxKernel(LinuxKernelFile))
    {
        UiMessageBox("Invalid Linux kernel!\n");
        goto LinuxBootFailed;
    }
    
    // Set up boot parameters
    BootParams = MmAllocateMemoryWithType(sizeof(*BootParams),
                                    LoaderSystemCode);
    if (!BootParams)
    {
        UiMessageBox("Cannot allocate Boot Args!\n");
        goto LinuxBootFailed;
    }
    
    TRACE("BootParams: %X\n", BootParams);
    
    RtlZeroMemory(BootParams, sizeof(*BootParams));
    SetupHeader = &BootParams->hdr;
    
    // Get the end of the setup header
    Position.QuadPart = 0x201;
    ArcSeek(LinuxKernelFile, &Position, SeekAbsolute);
    ArcRead(LinuxKernelFile, &SetupHeaderEnd, 1, NULL);
    SetupHeaderEnd += 0x202;
    
    // Copy the setup header to the boot params struct
    Position.QuadPart = 0x1F1;
    ArcSeek(LinuxKernelFile, &Position, SeekAbsolute);
    ArcRead(LinuxKernelFile, SetupHeader, SetupHeaderEnd - 0x1F1, NULL);
    
    // Check kernel version
    if (SetupHeader->version < 0x203)
    {
        UiMessageBox("Invalid or too old kernel");
        goto LinuxBootFailed;
    }
    
    if (!(SetupHeader->loadflags & LOADED_HIGH))
    {
        UiMessageBox("Kernels that load at 0x10000 are not supported!");
        goto LinuxBootFailed;
    }
    
    SetupHeader->cmd_line_ptr   = PtrToUlong(LinuxCommandLine);
    SetupHeader->loadflags      &= ~QUIET_FLAG;
    SetupHeader->vid_mode       = 0xFFFF;
    SetupHeader->type_of_loader = LINUX_LOADER_TYPE_FREELOADER;
    
    // Get the offset of the Linux kernel (real mode code size)
    Position.QuadPart = 0x1F1;
    ArcSeek(LinuxKernelFile, &Position, SeekAbsolute);
    ArcRead(LinuxKernelFile, &SetupCodeSize, 1, NULL);
    
    if (SetupCodeSize == 0)
        SetupCodeSize = 4;
    
    SetupCodeSize *= 512;
    LinuxKernelOffset = SetupCodeSize + 512;
    
    TRACE("Kernel offset: %d\n", LinuxKernelOffset);
    
    // Get the kernel size
    Status = ArcGetFileInformation(LinuxKernelFile, &FileInfo);
    if (Status != ESUCCESS || FileInfo.EndingAddress.HighPart != 0)
    {
        UiMessageBox("Could not get file size of Linux kernel!\n");
        goto LinuxBootFailed;
    }
    
    LinuxKernelSize = FileInfo.EndingAddress.LowPart - LinuxKernelOffset;
    TRACE("Kernel size: %d\n", LinuxKernelSize);
    
    // Get the ramdisk size
    if (LinuxInitrdName)
    {
        Status = ArcGetFileInformation(LinuxInitrdFile, &FileInfo);
        if (Status != ESUCCESS || FileInfo.EndingAddress.HighPart != 0)
        {
            LinuxInitrdSize = 0;
        }
        else
        {
            LinuxInitrdSize = FileInfo.EndingAddress.LowPart;
        }
        
        TRACE("InitrdSize: %d\n", LinuxInitrdSize);
    }
    
    TotalSize = LinuxKernelSize + LinuxInitrdSize;
    
    LinuxKernelLoadAddress = ReadLinuxKernel(LinuxKernelFile,
                                    LinuxKernelSize,
                                    LinuxKernelOffset, 
                                    LinuxKernelName);
    if (LinuxKernelLoadAddress == NULL)
    {
        UiMessageBox("Could not load Linux kernel!\n");
        goto LinuxBootFailed;
    }
    
    TRACE("Linux load address: %X\n", LinuxKernelLoadAddress);
    
    if (LinuxInitrdSize != 0)
    {
        LinuxInitrdLoadAddress = ReadLinuxInitrd(LinuxInitrdFile,
                                        LinuxInitrdSize,
                                        LinuxKernelSize,
                                        SetupHeader->initrd_addr_max,
                                        LinuxInitrdName);
        if (LinuxInitrdLoadAddress == NULL)
        {
            UiMessageBox("Could not load initial ramdisk!\n");
            goto LinuxBootFailed;
        }
        
        TRACE("Ramdisk load address: %X\n", LinuxInitrdLoadAddress);
        
        SetupHeader->ramdisk_image  = PtrToUlong(LinuxInitrdLoadAddress);
        SetupHeader->ramdisk_size   = LinuxInitrdSize;
    }

    ScreenInfo = &BootParams->screen_info;
    
#ifdef UEFIBOOT
    ScreenInfo->capabilities        = VIDEO_CAPABILITY_64BIT_BASE | VIDEO_CAPABILITY_SKIP_QUIRKS;
    ScreenInfo->flags               = VIDEO_FLAGS_NOCURSOR;
    ScreenInfo->orig_video_isVGA    = VIDEO_TYPE_EFI;
    
    ScreenInfo->lfb_base            = (UINT32) (UINT64) VramAddress;
    ScreenInfo->ext_lfb_base        = (UINT32) ((UINT64) VramAddress >> 32);
    ScreenInfo->lfb_size            = VramSize;
    ScreenInfo->lfb_width           = FrameBufferData->ScreenWidth;
    ScreenInfo->lfb_height          = FrameBufferData->ScreenHeight;
    ScreenInfo->lfb_depth           = 32;
    ScreenInfo->lfb_linelength      = FrameBufferData->PixelsPerScanLine * 4;
    
    LinearMaskToMaskShift(&ScreenInfo->red_size,
                    &ScreenInfo->red_pos,
                    FrameBufferData->PixelMasks.RedMask);
    LinearMaskToMaskShift(&ScreenInfo->green_size,
                    &ScreenInfo->green_pos,
                    FrameBufferData->PixelMasks.GreenMask);
    LinearMaskToMaskShift(&ScreenInfo->blue_size,
                    &ScreenInfo->blue_pos,
                    FrameBufferData->PixelMasks.BlueMask);
    LinearMaskToMaskShift(&ScreenInfo->rsvd_size,
                    &ScreenInfo->rsvd_pos,
                    FrameBufferData->PixelMasks.ReservedMask);
    
    BootParams->efi_info.efi_systab             = (UINT32) (UINT64) (UINT_PTR) GlobalSystemTable;
    BootParams->efi_info.efi_systab_hi          = (UINT32) ((UINT64) (UINT_PTR) GlobalSystemTable >> 32);
    BootParams->efi_info.efi_memmap             = (UINT32) (UINT64) (UINT_PTR) EfiMemoryMap;
    BootParams->efi_info.efi_memmap_hi          = (UINT32) ((UINT64) (UINT_PTR) EfiMemoryMap >> 32);
    BootParams->efi_info.efi_memmap_size        = (UINT32) MapSize;
    BootParams->efi_info.efi_memdesc_size       = (UINT32) DescriptorSize;
    BootParams->efi_info.efi_memdesc_version    = (UINT32) DescriptorVersion;
#else
    ScreenInfo->orig_video_mode     = 3;
    ScreenInfo->orig_video_ega_bx   = 3;
    ScreenInfo->orig_video_lines    = 25;
    ScreenInfo->orig_video_cols     = 80;
    ScreenInfo->orig_video_points   = 16;
    ScreenInfo->orig_video_isVGA    = VIDEO_TYPE_VGAC;
#endif
    BootParams->acpi_rsdp_addr      = (ULONG_PTR) FindAcpiBios();
    
    LinuxFillMemoryMap(BootParams->e820_table,
                    &BootParams->e820_entries);
    
    MachPrepareForReactOS();
    
#ifdef UEFIBOOT
#ifdef _M_IX86
    RtlCopyMemory(&BootParams->efi_info.efi_loader_signature, "EL32", 4);
#else
    RtlCopyMemory(&BootParams->efi_info.efi_loader_signature, "EL64", 4);
#endif
#endif
    
    BootLinuxKernel(LinuxKernelLoadAddress, BootParams);
    
LinuxBootFailed:
    if (LinuxKernelFile)
        ArcClose(LinuxKernelFile);

    if (LinuxInitrdFile)
        ArcClose(LinuxInitrdFile);

    if (LinuxKernelLoadAddress != NULL)
        MmFreeMemory(LinuxKernelLoadAddress);

    if (LinuxInitrdLoadAddress != NULL)
        MmFreeMemory(LinuxInitrdLoadAddress);
    
    if (BootParams)
        MmFreeMemory(BootParams);
    
    LinuxKernelLoadAddress = NULL;
    LinuxInitrdLoadAddress = NULL;
        
    return ENOEXEC;
}

static
BOOLEAN
ValidateLinuxKernel(ULONG File)
{
    LARGE_INTEGER       Position;
    ULONG               LinuxSignature;
    
    // Check the kernel signature
    Position.QuadPart = 0x202;
    ArcSeek(File, &Position, SeekAbsolute);
    ArcRead(File, &LinuxSignature, sizeof(ULONG), NULL);
    if (LinuxSignature != LINUX_SETUP_HEADER_ID)
    {
        UiMessageBox("Invalid signature! Expected 0x%X, got 0x%X",
                LINUX_SETUP_HEADER_ID,
                LinuxSignature);
        
        return FALSE;
    }
    
    return TRUE;
}

static
PVOID
ReadLinuxKernel(ULONG File, ULONG Size, ULONG Offset, PCSTR Name)
{
    LARGE_INTEGER   Position;
    PVOID           LoadAddress, CurrentLocation;
    ULONG           BytesLoaded;
    UINT_PTR        LoadIncrementer;
    CHAR            StatusText[260];
    
    RtlStringCbPrintfA(StatusText,
                sizeof(StatusText),
                "Loading %s",
                Name);
    UiDrawStatusText(StatusText);
    
    LoadIncrementer = LINUX_KERNEL_LOAD_ADDRESS;
    
    /*
     * Try to allocate memory for the Linux kernel, starting at 0x100000 (1MB).
     */
    
    LoadAddress = MmAllocateMemoryAtAddress(Size,
                        (PVOID) LoadIncrementer,
                        LoaderLoadedProgram);
    if (LoadAddress != (PVOID) LINUX_KERNEL_LOAD_ADDRESS)
    {
        for (;;)
        {
            // Put it up an additional megabyte in memory.
            LoadIncrementer += LINUX_KERNEL_LOAD_ADDRESS;
            LoadAddress = MmAllocateMemoryAtAddress(Size,
                                        (PVOID) LoadIncrementer,
                                        LoaderLoadedProgram);
                                        
            if (LoadAddress == (PVOID) LoadIncrementer)
            {
                break;
            }
        }
    }
    
    CurrentLocation = LoadAddress;
    
    Position.QuadPart = Offset;
    if (ArcSeek(File, &Position, SeekAbsolute) != ESUCCESS)
        return NULL;
    
    for (BytesLoaded = 0; BytesLoaded < Size; )
    {
        if (ArcRead(File, CurrentLocation, LINUX_READ_CHUNK_SIZE, NULL) != ESUCCESS)
            return FALSE;
        
        BytesLoaded += LINUX_READ_CHUNK_SIZE;
        CurrentLocation = (PVOID) ((ULONG_PTR) CurrentLocation + LINUX_READ_CHUNK_SIZE);
        
        UiUpdateProgressBar(BytesLoaded * 100 / TotalSize, NULL);
    }
    
    return LoadAddress;
}

static
PVOID
ReadLinuxInitrd(ULONG File, ULONG Size, ULONG KernelSize, ULONG MaxAddr, PCSTR Name)
{
    PVOID LoadAddress, CurrentLocation;
    ULONG BytesLoaded;
    CHAR  StatusText[260];

    
    RtlStringCbPrintfA(StatusText,
                sizeof(StatusText),
                "Loading %s",
                Name);
    UiDrawStatusText(StatusText);
    
    LoadAddress = MmAllocateHighestMemoryBelowAddress(Size,
                    ULongToPtr(MaxAddr),
                    LoaderLoadedProgram);
    
    CurrentLocation = LoadAddress;

    for (BytesLoaded = 0; BytesLoaded < Size; )
    {
        if (ArcRead(File, CurrentLocation, LINUX_READ_CHUNK_SIZE, NULL) != ESUCCESS)
            return NULL;
        
        BytesLoaded += LINUX_READ_CHUNK_SIZE;
        CurrentLocation = (PVOID) ((ULONG_PTR) CurrentLocation + LINUX_READ_CHUNK_SIZE);
        
        UiUpdateProgressBar((BytesLoaded + KernelSize) * 100 / TotalSize, NULL);
    }
    
    return LoadAddress;
}

#endif /* _M_IX86 || _M_AMD64 */