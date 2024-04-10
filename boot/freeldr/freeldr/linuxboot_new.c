/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     New Linux boot routines
 * COPYRIGHT:   Copyright 2024 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(LINUX);

/* GLOBALS *******************************************************************/

#define LINUX_READ_CHUNK_SIZE   0x20000 // Read 128k at a time

PCHAR   LinuxKernelVersion;
PVOID   LinuxKernelLoadAddress = NULL;
PVOID   LinuxInitrdLoadAddress = NULL;
ULONG   LinuxKernelSize = 0;
ULONG   LinuxInitrdSize = 0;
PCSTR   LinuxKernelName = NULL;
PCSTR   LinuxInitrdName = NULL;
PSTR    LinuxCommandLine = NULL;
SIZE_T  RealModeCodeSize = 0;
ULONG   TotalLoadPercentage = 0;
PLINUX_SETUP_HEADER SetupHeader = NULL;
PLINUX_BOOT_PARAMS  BootParams = NULL;
PLINUX_SCREEN_INFO  ScreenInfo = NULL;
PLINUX_E820_ENTRY   E820Table = NULL;

extern BIOS_MEMORY_MAP BiosMap[MAX_BIOS_DESCRIPTORS];
extern REACTOS_INTERNAL_BGCONTEXT framebufferData;
extern INT FreeldrDescCount;

PRSDP_DESCRIPTOR FindAcpiBios(VOID);
VOID CopySmbios(VOID);

/* FUNCTIONS *****************************************************************/

char * __cdecl strtok( char *str, const char *delim )
{
    char *ret;

    if (!str)
        return NULL;

    while (*str && strchr( delim, *str )) str++;
    if (!*str) return NULL;
    ret = str++;
    while (*str && !strchr( delim, *str )) str++;
    if (*str) *str++ = 0;
    return ret;
}

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

static BOOLEAN
ValidateLinuxKernel(ULONG LinuxKernel)
{
    SIZE_T SetupCodeSize = 0;
    SIZE_T SetupHeaderEnd = 0;

    LARGE_INTEGER Position;
    ULONG32 LinuxSignature;
    
    
    
    // Check the signature
    Position.QuadPart = 0x202;
    if (ArcSeek(LinuxKernel, &Position, SeekAbsolute))
    {
        ERR("ArcSeek failed!\n");
        return FALSE;
    }
    
    if (ArcRead(LinuxKernel, &LinuxSignature, sizeof(ULONG32), NULL))
    {
        ERR("ArcRead failed!\n");
        return FALSE;
    }
    
    if (LinuxSignature != LINUX_BOOT_MAGIC)
    {
        ERR("Invalid magic number!\n");
        UiMessageBox("Invalid magic number! (Expected 0x%08X, got 0x%08X)", LINUX_BOOT_MAGIC, LinuxSignature);
        return FALSE;
    }
    
    // Get the setup code size
    Position.QuadPart = 0x1F1;
    if (ArcSeek(LinuxKernel, &Position, SeekAbsolute))
    {
        ERR("ArcSeek failed!\n");
        return FALSE;
    }
    
    if (ArcRead(LinuxKernel, &SetupCodeSize, 1, NULL))
    {
        ERR("ArcRead failed!\n");
        return FALSE;
    }
    
    // If the number of setup sectors is not provided, it is 4
    if (SetupCodeSize == 0)
    {
        SetupCodeSize = 4;
    }
    
    SetupCodeSize *= 512; // Sector size == 512
    
    RealModeCodeSize = 512 + SetupCodeSize;
    
    // Set up boot parameters
    BootParams = MmAllocateMemoryWithType(sizeof(LINUX_BOOT_PARAMS),
                                            LoaderSystemCode);
                                            
    if (!BootParams)
    {
        ERR("Cannot allocate Boot Params!\n");
        return FALSE;
    }
    
    RtlZeroMemory(BootParams, sizeof(LINUX_BOOT_PARAMS));
    
    SetupHeader = &BootParams->SetupHeader;
    
    // Get the end of the setup header
    Position.QuadPart = 0x201;
    if (ArcSeek(LinuxKernel, &Position, SeekAbsolute))
    {
        ERR("ArcSeek failed!\n");
        return FALSE;
    }
    
    if (ArcRead(LinuxKernel, &SetupHeaderEnd, 1, NULL))
    {
        ERR("ArcRead failed!\n");
        return FALSE;
    }
    
    SetupHeaderEnd += 0x202;
    
    // Copy the setup header to the boot params struct
    Position.QuadPart = 0x1F1;
    if (ArcSeek(LinuxKernel, &Position, SeekAbsolute))
    {
        ERR("ArcSeek failed!\n");
        return FALSE;
    }
    
    if (ArcRead(LinuxKernel, SetupHeader, SetupHeaderEnd - 0x1F1, NULL))
    {
        ERR("ArcRead failed!\n");
        return FALSE;
    }
    
    // Check Linux boot protocol version
    // Absolute minimum is 2.03
    if (SetupHeader->BootProtocolVersion < 0x203)
    {
        UiMessageBox("Linux kernel too old! Minimum version is 2.4.18");
        return FALSE;
    }
    
    // Get kernel version string
    LinuxKernelVersion = MmAllocateMemoryWithType(128, LoaderSystemCode);
    if (!LinuxKernelVersion)
    {
        ERR("Failed to allocate version string!!\n");
        return FALSE;
    }
    
    if (SetupHeader->KernelVersion != 0)
    {
        Position.QuadPart = (SetupHeader->KernelVersion + 0x200);
        if (ArcSeek(LinuxKernel, &Position, SeekAbsolute))
        {
            ERR("ArcSeek failed!\n");
            return FALSE;
        }
    
        if (ArcRead(LinuxKernel, LinuxKernelVersion, 128, NULL))
        {
            ERR("ArcRead failed!\n");
            return FALSE;
        }
        
        CHAR ProgressString[256];
        RtlStringCbPrintfA(ProgressString,
                        sizeof(ProgressString),
                        "Loading Linux version %s...",
                        strtok(LinuxKernelVersion, " "));
        UiUpdateProgressBar(0, ProgressString);
    }
    MmFreeMemory(LinuxKernelVersion);
    
    if (!(SetupHeader->LoadFlags & (1 << 0)))
    {
        UiMessageBox("Kernels that load at 0x10000 are not supported!");
        return FALSE;
    }
    
    return TRUE;
}

static BOOLEAN
LinuxReadKernel(ULONG LinuxKernelFile)
{
    PVOID LoadAddress;
    LARGE_INTEGER Position;
    ULONG BytesLoaded;
    ULONG LinuxLoadPercentage;
    
    CHAR  StatusText[260];

    RtlStringCbPrintfA(StatusText, sizeof(StatusText), "Loading %s", LinuxKernelName);
    UiDrawStatusText(StatusText);
    
    UINT_PTR    LoadIncrememter = LINUX_KERNEL_LOAD_ADDRESS;
    
    /* Try to allocate memory for the Linux kernel; if it fails, allocate somewhere else */
    LinuxKernelLoadAddress = MmAllocateMemoryAtAddress(LinuxKernelSize, (PVOID)LoadIncrememter, LoaderSystemCode);
    if (LinuxKernelLoadAddress != (PVOID)LINUX_KERNEL_LOAD_ADDRESS)
    {
        for (;;)
        {
            // Put it an additional megabyte up
            LinuxKernelLoadAddress = MmAllocateMemoryAtAddress(LinuxKernelSize, (PVOID) LoadIncrememter, LoaderSystemCode);
            if (LinuxKernelLoadAddress)
            {
                break;
            }
            LoadIncrememter += LINUX_KERNEL_LOAD_ADDRESS;
        }
    }

    LoadAddress = LinuxKernelLoadAddress;

    /* Load the linux kernel at 0x100000 (1mb) */
    Position.QuadPart = RealModeCodeSize;
    if (ArcSeek(LinuxKernelFile, &Position, SeekAbsolute) != ESUCCESS)
        return FALSE;
    for (BytesLoaded = 0; BytesLoaded < LinuxKernelSize; )
    {
        if (ArcRead(LinuxKernelFile, LoadAddress, LINUX_READ_CHUNK_SIZE, NULL) != ESUCCESS)
            return FALSE;

        BytesLoaded += LINUX_READ_CHUNK_SIZE;
        LoadAddress = (PVOID)((ULONG_PTR)LoadAddress + LINUX_READ_CHUNK_SIZE);

        LinuxLoadPercentage = BytesLoaded * 100 / (LinuxInitrdSize + LinuxKernelSize);

        UiUpdateProgressBar(TotalLoadPercentage + LinuxLoadPercentage, NULL);
    }
    
    TotalLoadPercentage += LinuxLoadPercentage;

    return TRUE;
}

static BOOLEAN LinuxReadInitrd(ULONG LinuxInitrdFile)
{
    ULONG BytesLoaded;
    CHAR  StatusText[260];
    ULONG InitrdLoadPercentage;

    RtlStringCbPrintfA(StatusText, sizeof(StatusText), "Loading %s", LinuxInitrdName);
    UiDrawStatusText(StatusText);

    /* Allocate memory for the ramdisk, below 4GB */
    LinuxInitrdLoadAddress = MmAllocateHighestMemoryBelowAddress(LinuxInitrdSize,
                                                UlongToPtr(SetupHeader->MaxInitrdAddr),
                                                LoaderSystemCode);
    if (LinuxInitrdLoadAddress == NULL)
    {
        return FALSE;
    }

    /* Set the information in the setup struct */
    SetupHeader->RamdiskImage = PtrToUlong(LinuxInitrdLoadAddress);
    SetupHeader->RamdiskSize = LinuxInitrdSize;

    /* Load the ramdisk */
    for (BytesLoaded = 0; BytesLoaded < LinuxInitrdSize; )
    {
        if (ArcRead(LinuxInitrdFile, LinuxInitrdLoadAddress, LINUX_READ_CHUNK_SIZE, NULL) != ESUCCESS)
            return FALSE;

        BytesLoaded += LINUX_READ_CHUNK_SIZE;
        LinuxInitrdLoadAddress = (PVOID)((ULONG_PTR)LinuxInitrdLoadAddress + LINUX_READ_CHUNK_SIZE);
        
        InitrdLoadPercentage = BytesLoaded * 100 / (LinuxInitrdSize + LinuxKernelSize);
        
        UiUpdateProgressBar(TotalLoadPercentage + InitrdLoadPercentage, "Loading initial ramdisk...");
    }
    
    TotalLoadPercentage += InitrdLoadPercentage;

    return TRUE;
}

ARC_STATUS
LoadAndBootLinux(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[])
{    
    ARC_STATUS Status;
    PCSTR ArgValue;
    PCSTR BootPath;
    UCHAR DriveNumber = 0;
    ULONG PartitionNumber = 0;
    ULONG LinuxKernel = 0;
    ULONG LinuxInitrd = 0;
    FILEINFORMATION FileInfo;
        
    CHAR ArcPath[MAX_PATH];
    
    // Start boot
    UiDrawBackdrop();
    UiDrawStatusText("Loading Linux...");
    UiDrawProgressBarCenter("Loading Linux...");
    
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
            DriveNumber = DriveMapGetBiosDriveNumber(ArgValue);

            /* Retrieve the boot partition (not optional and cannot be zero) */
            PartitionNumber = 0;
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
    
    /* If we haven't retrieved the BIOS drive and partition numbers above, do it now */
    if (PartitionNumber == 0)
    {
        /* Retrieve the BIOS drive and partition numbers */
        if (!DissectArcPath(BootPath, NULL, &DriveNumber, &PartitionNumber))
        {
            /* This is not a fatal failure, but just an inconvenience: display a message */
            TRACE("DissectArcPath(%s) failed to retrieve BIOS drive and partition numbers.\n", BootPath);
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
    Status = FsOpenFile(LinuxKernelName, BootPath, OpenReadOnly, &LinuxKernel);
    if (Status != ESUCCESS)
    {
        UiMessageBox("Linux kernel '%s' not found.", LinuxKernelName);
        goto LinuxBootFailed;
    }
    
    /* Open the initrd file image (if necessary) */
    if (LinuxInitrdName)
    {
        Status = FsOpenFile(LinuxInitrdName, BootPath, OpenReadOnly, &LinuxInitrd);
        if (Status != ESUCCESS)
        {
            UiMessageBox("Linux initrd image '%s' not found.", LinuxInitrdName);
            goto LinuxBootFailed;
        }
    }
    
    // Validate Linux kernel and set up BootParams
    if (!ValidateLinuxKernel(LinuxKernel))
    {
        UiMessageBox("Failed to validate Linux kernel");
        goto LinuxBootFailed;
    }
    
    // Add some other stuff to the setup header
    SetupHeader->CommandLine = (ULONG) (UINT_PTR) LinuxCommandLine;
    SetupHeader->LoadFlags &= ~(1 << 5); // print early messages
    SetupHeader->VideoMode = 0xFFFF;
    SetupHeader->TypeOfLoader = 0xFF;
    
    // Load the kernel into memory
    Status = ArcGetFileInformation(LinuxKernel, &FileInfo);
    if (Status != ESUCCESS || FileInfo.EndingAddress.HighPart != 0)
    {
        LinuxKernelSize = 0;
    }
    else
    {
        LinuxKernelSize = FileInfo.EndingAddress.LowPart - RealModeCodeSize;
    }
    
    // Load the initrd into memory
    if (LinuxInitrdName)
    {
        Status = ArcGetFileInformation(LinuxInitrd, &FileInfo);
        
        if (Status != ESUCCESS || FileInfo.EndingAddress.HighPart != 0)
        {
            LinuxInitrdSize = 0;
        }
        else
        {
            LinuxInitrdSize = FileInfo.EndingAddress.LowPart;
        }
    }
    
    if (!LinuxReadKernel(LinuxKernel))
    {
        goto LinuxBootFailed;
    }
    
    // Load the initrd into memory 
    if (!LinuxReadInitrd(LinuxInitrd))
    {
        goto LinuxBootFailed;
    }
    
    // Set up EFI stuff
    // Currently reliant on Apple TV code, for now
    ScreenInfo = &BootParams->ScreenInfo;
    
    ScreenInfo->Capabilities        = (1 << 1) | (1 << 0); // skip quirks
    ScreenInfo->Flags               = (1 << 0); // no cursor
    ScreenInfo->FbBase              = (UINT32) (UINT64) framebufferData.BaseAddress;
    ScreenInfo->ExtendedFbBase      = (UINT32) ((UINT64) framebufferData.BaseAddress >> 32);
    ScreenInfo->FbSize              = framebufferData.PixelsPerScanLine * framebufferData.ScreenHeight * 4;
    ScreenInfo->FbWidth             = framebufferData.ScreenWidth;
    ScreenInfo->FbHeight            = framebufferData.ScreenHeight;
    ScreenInfo->FbDepth             = 32;
    ScreenInfo->FbLineLength        = framebufferData.PixelsPerScanLine * 4;
    ScreenInfo->RedSize             = 8;
    ScreenInfo->RedPosition         = 16;
    ScreenInfo->GreenSize           = 8;
    ScreenInfo->GreenPosition       = 8;
    ScreenInfo->BlueSize            = 8;
    ScreenInfo->BluePosition        = 0;
    ScreenInfo->ReservedSize        = 8;
    ScreenInfo->ReservedPosition    = 24;
    
    ScreenInfo->OrigVideoIsVGA      = VIDEO_TYPE_EFI;
    // ACPI
    BootParams->Rsdp    = (UINT_PTR) FindAcpiBios();
    
    // EFI
    #ifdef UEFIBOOT
    RtlCopyMemory(&BootParams->EfiInfo.EfiLoaderSignature, "EL32", 4);
    
    
    BootParams->EfiInfo.EfiSystemTable              = (UINT32) (UINT64) (UINT_PTR) BootArgs->EfiSystemTable;
    BootParams->EfiInfo.EfiSystemTableHigh          = (UINT32) ((UINT64) (UINT_PTR) BootArgs->EfiSystemTable >> 32);
    BootParams->EfiInfo.EfiMemoryMap                = (UINT32) (UINT64) (UINT_PTR) BootArgs->EfiMemoryMap;
    BootParams->EfiInfo.EfiMemoryMapHigh            = (UINT32) ((UINT64) (UINT_PTR) BootArgs->EfiMemoryMap >> 32);
    BootParams->EfiInfo.EfiMemoryMapSize            = BootArgs->EfiMemoryMapSize;
    BootParams->EfiInfo.EfiMemoryDescriptorSize     = BootArgs->EfiMemoryDescriptorSize;
    BootParams->EfiInfo.EfiMemoryDescriptorVersion  = BootArgs->EfiMemoryDescriptorVersion;
    */
    #endif
    
    // E820 memory map
    E820Table = MmAllocateMemoryWithType(LINUX_E820_MAX * sizeof(LINUX_E820_ENTRY), LoaderSystemCode);
    
    for (SIZE_T i = 0, j = 0; i < FreeldrDescCount; i++)
    {
        if (BiosMap[i].Type >= 0x1000)
        {
            continue;
        }
        E820Table[i].Address    = BiosMap[i].BaseAddress;
        E820Table[i].Size       = BiosMap[i].Length;
        E820Table[i].Type       = BiosMap[i].Type;
        j++;
        BootParams->E820Entries = j;
    }
    
    RtlCopyMemory(&BootParams->E820Table, E820Table, LINUX_E820_MAX * sizeof(LINUX_E820_ENTRY));
    
    MmFreeMemory(E820Table);
    
    CopySmbios();
    
    UiUpdateProgressBar(100, "Starting kernel...");
    
    // Hack start until we have something better.
    asm("jmp *%0"::"r"(LinuxKernelLoadAddress), "S"(BootParams));
    
    // Should never return
    return ESUCCESS;

    
LinuxBootFailed:
    
    // Free everything
    if (LinuxKernel)
        ArcClose(LinuxKernel);
    
    if (LinuxInitrd)
        ArcClose(LinuxInitrd);
    
    if (!BootParams)
        MmFreeMemory(BootParams);
    
    if (!LinuxKernelLoadAddress)
        MmFreeMemory(LinuxKernelLoadAddress);
    
    if (!LinuxInitrdLoadAddress)
        MmFreeMemory(LinuxInitrdLoadAddress);
    
    BootParams = NULL;
    LinuxKernelVersion = NULL;
    LinuxKernelLoadAddress = NULL;
    LinuxCommandLine = NULL;
    LinuxKernelSize = 0;
    LinuxInitrdSize = 0;
    LinuxKernelName = NULL;
    LinuxInitrdName = NULL;
    LinuxCommandLine = NULL;
    RealModeCodeSize = 0;
    
    return ENOEXEC;
}