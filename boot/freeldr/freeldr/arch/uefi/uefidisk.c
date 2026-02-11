/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Disk Access Functions (UEFI Spec Compliant)
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <uefildr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

#define TAG_HW_RESOURCE_LIST    'lRwH'
#define TAG_HW_DISK_CONTEXT     'cDwH'
#define FIRST_BIOS_DISK 0x80
#define FIRST_PARTITION 1

/* Maximum block size we support (8KB) - filters out flash devices */
#define MAX_SUPPORTED_BLOCK_SIZE 8192

typedef struct tagDISKCONTEXT
{
    UCHAR DriveNumber;
    ULONG SectorSize;
    ULONGLONG SectorOffset;
    ULONGLONG SectorCount;
    ULONGLONG SectorNumber;
} DISKCONTEXT;

typedef struct _INTERNAL_UEFI_DISK
{
    UCHAR ArcDriveNumber;
    UCHAR NumOfPartitions;
    ULONG UefiHandleIndex;      /* Index into handles array */
    BOOLEAN IsThisTheBootDrive;
    EFI_HANDLE Handle;           /* Store handle for direct access */
} INTERNAL_UEFI_DISK, *PINTERNAL_UEFI_DISK;

/* GLOBALS *******************************************************************/

extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
extern EFI_HANDLE PublicBootHandle; /* Freeldr itself */

/* Made to match BIOS */
PVOID DiskReadBuffer;
UCHAR PcBiosDiskCount;

UCHAR FrldrBootDrive;
ULONG FrldrBootPartition;
SIZE_T DiskReadBufferSize;
PVOID Buffer;

static const CHAR Hex[] = "0123456789abcdef";
static CHAR PcDiskIdentifier[32][20];

/* UEFI-specific */
static ULONG UefiBootRootIndex = 0;
static ULONG PublicBootArcDisk = 0;
static INTERNAL_UEFI_DISK* InternalUefiDisk = NULL;
static EFI_GUID BlockIoGuid = EFI_BLOCK_IO_PROTOCOL_GUID;
static EFI_HANDLE* handles = NULL;
static ULONG HandleCount = 0;

/* FUNCTIONS *****************************************************************/

PCHAR
GetHarddiskIdentifier(UCHAR DriveNumber)
{
    TRACE("GetHarddiskIdentifier: DriveNumber: %d\n", DriveNumber);
    if (DriveNumber < FIRST_BIOS_DISK)
        return NULL;
    return PcDiskIdentifier[DriveNumber - FIRST_BIOS_DISK];
}

static LONG lReportError = 0; // >= 0: display errors; < 0: hide errors.

LONG
DiskReportError(BOOLEAN bShowError)
{
    /* Set the reference count */
    if (bShowError) ++lReportError;
    else            --lReportError;
    return lReportError;
}

static
BOOLEAN
UefiGetBootPartitionEntry(
    IN UCHAR DriveNumber,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry,
    OUT PULONG BootPartition)
{
    ULONG PartitionNum;
    ULONG ArcDriveIndex;

    TRACE("UefiGetBootPartitionEntry: DriveNumber: %d\n", DriveNumber - FIRST_BIOS_DISK);
    
    if (DriveNumber < FIRST_BIOS_DISK)
        return FALSE;
    
    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
    if (ArcDriveIndex >= PcBiosDiskCount)
        return FALSE;

    /* Find which partition handle corresponds to the boot handle */
    /* The boot handle should be a partition on the boot disk */
    /* For now, we'll use partition 1 as default if we can't determine */
    PartitionNum = FIRST_PARTITION;
    
    /* TODO: Properly determine partition number from boot handle */
    *BootPartition = PartitionNum;
    TRACE("UefiGetBootPartitionEntry: Boot Partition is: %d\n", PartitionNum);
    return TRUE;
}

static
ARC_STATUS
UefiDiskClose(ULONG FileId)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    if (Context)
    {
        FrLdrTempFree(Context, TAG_HW_DISK_CONTEXT);
    }
    return ESUCCESS;
}

static
ARC_STATUS
UefiDiskGetFileInformation(ULONG FileId, FILEINFORMATION *Information)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    if (!Context)
        return EINVAL;
    
    RtlZeroMemory(Information, sizeof(*Information));

    /*
     * The ARC specification mentions that for partitions, StartingAddress and
     * EndingAddress are the start and end positions of the partition in terms
     * of byte offsets from the start of the disk.
     * CurrentAddress is the current offset into (i.e. relative to) the partition.
     */
    Information->StartingAddress.QuadPart = Context->SectorOffset * Context->SectorSize;
    Information->EndingAddress.QuadPart   = (Context->SectorOffset + Context->SectorCount) * Context->SectorSize;
    Information->CurrentAddress.QuadPart  = Context->SectorNumber * Context->SectorSize;

    Information->Type = DiskPeripheral; /* No floppy for you for now... */

    return ESUCCESS;
}

static
ARC_STATUS
UefiDiskOpen(CHAR *Path, OPENMODE OpenMode, ULONG *FileId)
{
    DISKCONTEXT* Context;
    UCHAR DriveNumber;
    ULONG DrivePartition, SectorSize;
    ULONGLONG SectorOffset = 0;
    ULONGLONG SectorCount = 0;
    ULONG ArcDriveIndex;
    PARTITION_TABLE_ENTRY PartitionTableEntry;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;

    TRACE("UefiDiskOpen: File ID: %p, Path: %s\n", FileId, Path);

    if (DiskReadBufferSize == 0)
    {
        ERR("DiskOpen(): DiskReadBufferSize is 0, something is wrong.\n");
        ASSERT(FALSE);
        return ENOMEM;
    }

    if (!DissectArcPath(Path, NULL, &DriveNumber, &DrivePartition))
        return EINVAL;

    TRACE("Opening disk: DriveNumber: %d, DrivePartition: %d\n", DriveNumber, DrivePartition);
    
    if (DriveNumber < FIRST_BIOS_DISK)
        return EINVAL;
    
    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
    if (ArcDriveIndex >= PcBiosDiskCount || InternalUefiDisk == NULL)
        return EINVAL;

    /* Get Block I/O protocol for this drive */
    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);
    
    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for drive %d\n", DriveNumber);
        return EINVAL;
    }

    /* Check media is present */
    if (!BlockIo->Media->MediaPresent)
    {
        ERR("Media not present for drive %d\n", DriveNumber);
        return EINVAL;
    }

    SectorSize = BlockIo->Media->BlockSize;

    if (DrivePartition != 0xff && DrivePartition != 0)
    {
        if (!DiskGetPartitionEntry(DriveNumber, DrivePartition, &PartitionTableEntry))
            return EINVAL;

        SectorOffset = PartitionTableEntry.SectorCountBeforePartition;
        SectorCount = PartitionTableEntry.PartitionSectorCount;
    }
    else
    {
        GEOMETRY Geometry;
        if (!MachDiskGetDriveGeometry(DriveNumber, &Geometry))
            return EINVAL;

        if (SectorSize != Geometry.BytesPerSector)
        {
            ERR("SectorSize (%lu) != Geometry.BytesPerSector (%lu), expect problems!\n",
                SectorSize, Geometry.BytesPerSector);
        }

        SectorOffset = 0;
        SectorCount = Geometry.Sectors;
    }

    Context = FrLdrTempAlloc(sizeof(DISKCONTEXT), TAG_HW_DISK_CONTEXT);
    if (!Context)
        return ENOMEM;

    Context->DriveNumber = DriveNumber;
    Context->SectorSize = SectorSize;
    Context->SectorOffset = SectorOffset;
    Context->SectorCount = SectorCount;
    Context->SectorNumber = 0;
    FsSetDeviceSpecific(*FileId, Context);
    return ESUCCESS;
}

static
ARC_STATUS
UefiDiskRead(ULONG FileId, VOID *Buffer, ULONG N, ULONG *Count)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    UCHAR* Ptr = (UCHAR*)Buffer;
    ULONG Length, TotalSectors, MaxSectors, ReadSectors;
    ULONGLONG SectorOffset;
    BOOLEAN ret;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;
    ULONG ArcDriveIndex;

    ASSERT(DiskReadBufferSize > 0);
    
    if (!Context)
        return EINVAL;

    TotalSectors = (N + Context->SectorSize - 1) / Context->SectorSize;
    MaxSectors   = DiskReadBufferSize / Context->SectorSize;
    SectorOffset = Context->SectorOffset + Context->SectorNumber;

    // If MaxSectors is 0, this will lead to infinite loop.
    // In release builds assertions are disabled, however we also have sanity checks in DiskOpen()
    ASSERT(MaxSectors > 0);
    
    if (MaxSectors == 0)
    {
        ERR("MaxSectors is 0, cannot read\n");
        *Count = 0;
        return EIO;
    }

    ArcDriveIndex = Context->DriveNumber - FIRST_BIOS_DISK;
    if (ArcDriveIndex >= PcBiosDiskCount || InternalUefiDisk == NULL)
    {
        ERR("Invalid drive number %d\n", Context->DriveNumber);
        *Count = 0;
        return EINVAL;
    }

    /* Get Block I/O protocol */
    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);
    
    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol\n");
        *Count = 0;
        return EIO;
    }

    ret = TRUE;

    while (TotalSectors)
    {
        ReadSectors = min(TotalSectors, MaxSectors);

        /* Use UEFI ReadBlocks directly with proper error checking */
        Status = BlockIo->ReadBlocks(
            BlockIo,
            BlockIo->Media->MediaId,
            SectorOffset,
            ReadSectors * Context->SectorSize,
            DiskReadBuffer);
        
        if (EFI_ERROR(Status))
        {
            ERR("ReadBlocks failed: Status = 0x%lx\n", (ULONG)Status);
            ret = FALSE;
            break;
        }

        Length = ReadSectors * Context->SectorSize;
        Length = min(Length, N);

        RtlCopyMemory(Ptr, DiskReadBuffer, Length);

        Ptr += Length;
        N -= Length;
        SectorOffset += ReadSectors;
        TotalSectors -= ReadSectors;
    }

    *Count = (ULONG)((ULONG_PTR)Ptr - (ULONG_PTR)Buffer);
    Context->SectorNumber = SectorOffset - Context->SectorOffset;

    return (ret ? ESUCCESS : EIO);
}

static
ARC_STATUS
UefiDiskSeek(ULONG FileId, LARGE_INTEGER *Position, SEEKMODE SeekMode)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    LARGE_INTEGER NewPosition = *Position;

    if (!Context)
        return EINVAL;

    switch (SeekMode)
    {
        case SeekAbsolute:
            break;
        case SeekRelative:
            NewPosition.QuadPart += (Context->SectorNumber * Context->SectorSize);
            break;
        default:
            ASSERT(FALSE);
            return EINVAL;
    }

    if (NewPosition.QuadPart & (Context->SectorSize - 1))
        return EINVAL;

    /* Convert in number of sectors */
    NewPosition.QuadPart /= Context->SectorSize;

    /* HACK: CDROMs may have a SectorCount of 0 */
    if (Context->SectorCount != 0 && NewPosition.QuadPart >= Context->SectorCount)
        return EINVAL;

    Context->SectorNumber = NewPosition.QuadPart;
    return ESUCCESS;
}

static const DEVVTBL UefiDiskVtbl =
{
    UefiDiskClose,
    UefiDiskGetFileInformation,
    UefiDiskOpen,
    UefiDiskRead,
    UefiDiskSeek,
};

static
VOID
GetHarddiskInformation(UCHAR DriveNumber)
{
    PMASTER_BOOT_RECORD Mbr;
    PULONG Buffer;
    ULONG i;
    ULONG Checksum;
    ULONG Signature;
    BOOLEAN ValidPartitionTable;
    CHAR ArcName[MAX_PATH];
    PARTITION_TABLE_ENTRY PartitionTableEntry;
    ULONG ArcDriveIndex;
    PCHAR Identifier;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
    if (ArcDriveIndex >= 32)
        return;
    
    Identifier = PcDiskIdentifier[ArcDriveIndex];

    /* Detect disk partition type */
    DiskDetectPartitionType(DriveNumber);

    /* Read the MBR */
    if (!MachDiskReadLogicalSectors(DriveNumber, 0ULL, 1, DiskReadBuffer))
    {
        ERR("Reading MBR failed\n");
        /* We failed, use a default identifier */
        sprintf(Identifier, "BIOSDISK%d", ArcDriveIndex);
        return;
    }

    Buffer = (ULONG*)DiskReadBuffer;
    Mbr = (PMASTER_BOOT_RECORD)DiskReadBuffer;

    Signature = Mbr->Signature;
    TRACE("Signature: %x\n", Signature);

    /* Calculate the MBR checksum */
    Checksum = 0;
    for (i = 0; i < 512 / sizeof(ULONG); i++)
    {
        Checksum += Buffer[i];
    }
    Checksum = ~Checksum + 1;
    TRACE("Checksum: %x\n", Checksum);

    ValidPartitionTable = (Mbr->MasterBootRecordMagic == 0xAA55);

    /* Fill out the ARC disk block */
    sprintf(ArcName, "multi(0)disk(0)rdisk(%u)", ArcDriveIndex);
    AddReactOSArcDiskInfo(ArcName, Signature, Checksum, ValidPartitionTable);

    sprintf(ArcName, "multi(0)disk(0)rdisk(%u)partition(0)", ArcDriveIndex);
    FsRegisterDevice(ArcName, &UefiDiskVtbl);

    /* Add partitions */
    i = FIRST_PARTITION;
    DiskReportError(FALSE);
    while (DiskGetPartitionEntry(DriveNumber, i, &PartitionTableEntry))
    {
        if (PartitionTableEntry.SystemIndicator != PARTITION_ENTRY_UNUSED)
        {
            sprintf(ArcName, "multi(0)disk(0)rdisk(%u)partition(%lu)", ArcDriveIndex, i);
            FsRegisterDevice(ArcName, &UefiDiskVtbl);
        }
        i++;
    }
    DiskReportError(TRUE);

    if (ArcDriveIndex < PcBiosDiskCount && InternalUefiDisk != NULL)
    {
        InternalUefiDisk[ArcDriveIndex].NumOfPartitions = i;
    }
    
    /* Convert checksum and signature to identifier string */
    Identifier[0] = Hex[(Checksum >> 28) & 0x0F];
    Identifier[1] = Hex[(Checksum >> 24) & 0x0F];
    Identifier[2] = Hex[(Checksum >> 20) & 0x0F];
    Identifier[3] = Hex[(Checksum >> 16) & 0x0F];
    Identifier[4] = Hex[(Checksum >> 12) & 0x0F];
    Identifier[5] = Hex[(Checksum >> 8) & 0x0F];
    Identifier[6] = Hex[(Checksum >> 4) & 0x0F];
    Identifier[7] = Hex[Checksum & 0x0F];
    Identifier[8] = '-';
    Identifier[9] = Hex[(Signature >> 28) & 0x0F];
    Identifier[10] = Hex[(Signature >> 24) & 0x0F];
    Identifier[11] = Hex[(Signature >> 20) & 0x0F];
    Identifier[12] = Hex[(Signature >> 16) & 0x0F];
    Identifier[13] = Hex[(Signature >> 12) & 0x0F];
    Identifier[14] = Hex[(Signature >> 8) & 0x0F];
    Identifier[15] = Hex[(Signature >> 4) & 0x0F];
    Identifier[16] = Hex[Signature & 0x0F];
    Identifier[17] = '-';
    Identifier[18] = (ValidPartitionTable ? 'A' : 'X');
    Identifier[19] = 0;
    TRACE("Identifier: %s\n", Identifier);
}

static
VOID
UefiSetupBlockDevices(VOID)
{
    ULONG BlockDeviceIndex;
    ULONG SystemHandleCount;
    EFI_STATUS Status;
    ULONG i;
    UINTN HandleSize = 0;
    EFI_BLOCK_IO* BlockIo;

    PcBiosDiskCount = 0;
    UefiBootRootIndex = 0;

    /* Step 1: Get the size needed for handles buffer */
    /* CRITICAL: First call must pass NULL for buffer (UEFI spec requirement) */
    Status = GlobalSystemTable->BootServices->LocateHandle(
        ByProtocol,
        &BlockIoGuid,
        NULL,
        &HandleSize,
        NULL);
    
    if (Status != EFI_BUFFER_TOO_SMALL)
    {
        ERR("Failed to get handle buffer size: Status = 0x%lx\n", (ULONG)Status);
        return;
    }

    SystemHandleCount = HandleSize / sizeof(EFI_HANDLE);
    if (SystemHandleCount == 0)
    {
        ERR("No block devices found\n");
        return;
    }

    /* Step 2: Allocate buffer for handles */
    handles = MmAllocateMemoryWithType(HandleSize, LoaderFirmwareTemporary);
    if (handles == NULL)
    {
        ERR("Failed to allocate memory for handles\n");
        return;
    }

    /* Step 3: Get actual handles */
    Status = GlobalSystemTable->BootServices->LocateHandle(
        ByProtocol,
        &BlockIoGuid,
        NULL,
        &HandleSize,
        handles);
    
    if (EFI_ERROR(Status))
    {
        ERR("Failed to locate block device handles: Status = 0x%lx\n", (ULONG)Status);
        return;
    }

    HandleCount = SystemHandleCount;

    /* Step 4: Allocate internal disk structure */
    InternalUefiDisk = MmAllocateMemoryWithType(
        sizeof(INTERNAL_UEFI_DISK) * SystemHandleCount,
        LoaderFirmwareTemporary);
    
    if (InternalUefiDisk == NULL)
    {
        ERR("Failed to allocate memory for internal disk structure\n");
        return;
    }

    RtlZeroMemory(InternalUefiDisk, sizeof(INTERNAL_UEFI_DISK) * SystemHandleCount);

    /* Step 5: Find boot handle and determine if it's a root device or partition */
    UefiBootRootIndex = 0;
    for (i = 0; i < SystemHandleCount; i++)
    {
        if (handles[i] == PublicBootHandle)
        {
            UefiBootRootIndex = i;
            TRACE("Found boot handle at index %lu\n", i);
            
            /* Check if boot handle is a root device or partition */
            Status = GlobalSystemTable->BootServices->HandleProtocol(
                handles[i],
                &BlockIoGuid,
                (VOID**)&BlockIo);
            
            if (!EFI_ERROR(Status) && BlockIo != NULL)
            {
                TRACE("Boot handle: LogicalPartition=%s, RemovableMedia=%s, BlockSize=%lu\n",
                    BlockIo->Media->LogicalPartition ? "TRUE" : "FALSE",
                    BlockIo->Media->RemovableMedia ? "TRUE" : "FALSE",
                    BlockIo->Media->BlockSize);
            }
            break;
        }
    }

    /* Step 6: Enumerate root block devices (skip logical partitions) */
    BlockDeviceIndex = 0;
    for (i = 0; i < SystemHandleCount; i++)
    {
        Status = GlobalSystemTable->BootServices->HandleProtocol(
            handles[i],
            &BlockIoGuid,
            (VOID**)&BlockIo);
        
        if (EFI_ERROR(Status))
        {
            TRACE("HandleProtocol failed for handle %lu: Status = 0x%lx\n", i, (ULONG)Status);
            continue;
        }

        if (BlockIo == NULL)
        {
            TRACE("BlockIo is NULL for handle %lu\n", i);
            continue;
        }

        /* UEFI Spec: Check MediaPresent before using device */
        if (!BlockIo->Media->MediaPresent)
        {
            TRACE("Media not present for handle %lu\n", i);
            continue;
        }

        /* UEFI Spec: BlockSize must be > 0 */
        if (BlockIo->Media->BlockSize == 0)
        {
            TRACE("Invalid block size (0) for handle %lu\n", i);
            continue;
        }

        /* Filter out devices with unusually large block sizes (flash devices) */
        if (BlockIo->Media->BlockSize > MAX_SUPPORTED_BLOCK_SIZE)
        {
            TRACE("Block size too large (%lu) for handle %lu, skipping\n",
                BlockIo->Media->BlockSize, i);
            continue;
        }

        /* UEFI Spec: Only process root devices (LogicalPartition == FALSE) */
        /* Logical partitions are handled separately by partition scanning */
        if (BlockIo->Media->LogicalPartition)
        {
            /* If boot handle is a logical partition, we need to find its parent root device */
            /* For now, we'll handle this after enumeration by matching handles */
            TRACE("Skipping logical partition handle %lu\n", i);
            continue;
        }

        /* This is a root block device */
        TRACE("Found root block device at index %lu: BlockSize=%lu, LastBlock=%llu\n",
            i, BlockIo->Media->BlockSize, BlockIo->Media->LastBlock);

        InternalUefiDisk[BlockDeviceIndex].ArcDriveNumber = BlockDeviceIndex;
        InternalUefiDisk[BlockDeviceIndex].UefiHandleIndex = i;
        InternalUefiDisk[BlockDeviceIndex].Handle = handles[i];
        InternalUefiDisk[BlockDeviceIndex].IsThisTheBootDrive = FALSE;

        /* Check if this root device contains the boot partition */
        /* If boot handle is a logical partition, we need to find which root device it belongs to */
        /* For now, if boot handle index matches, mark it as boot device */
        if (i == UefiBootRootIndex)
        {
            InternalUefiDisk[BlockDeviceIndex].IsThisTheBootDrive = TRUE;
            PublicBootArcDisk = BlockDeviceIndex;
            TRACE("Boot device is at ARC drive index %lu (root device)\n", BlockDeviceIndex);
        }

        /* Increment PcBiosDiskCount BEFORE calling GetHarddiskInformation
         * so that UefiDiskReadLogicalSectors can validate the drive number */
        PcBiosDiskCount = BlockDeviceIndex + 1;
        
        TRACE("Calling GetHarddiskInformation for drive %d (BlockDeviceIndex=%lu)\n",
            BlockDeviceIndex + FIRST_BIOS_DISK, BlockDeviceIndex);
        
        GetHarddiskInformation(BlockDeviceIndex + FIRST_BIOS_DISK);
        BlockDeviceIndex++;
    }
    
    /* Step 7: If boot handle was a logical partition, find its parent root device */
    if (UefiBootRootIndex < HandleCount)
    {
        Status = GlobalSystemTable->BootServices->HandleProtocol(
            handles[UefiBootRootIndex],
            &BlockIoGuid,
            (VOID**)&BlockIo);
        
        if (!EFI_ERROR(Status) && BlockIo != NULL && BlockIo->Media->LogicalPartition)
        {
            TRACE("Boot handle is a logical partition, searching for parent root device\n");
            TRACE("Boot partition: BlockSize=%lu, RemovableMedia=%s\n",
                BlockIo->Media->BlockSize,
                BlockIo->Media->RemovableMedia ? "TRUE" : "FALSE");
            
            /* Find the root device that matches the boot partition's characteristics */
            /* For CD-ROMs: match BlockSize=2048 and RemovableMedia=TRUE */
            /* For hard disks: match BlockSize and find the root device before this partition */
            BOOLEAN FoundBootDevice = FALSE;
            for (i = 0; i < BlockDeviceIndex; i++)
            {
                EFI_BLOCK_IO* RootBlockIo;
                Status = GlobalSystemTable->BootServices->HandleProtocol(
                    InternalUefiDisk[i].Handle,
                    &BlockIoGuid,
                    (VOID**)&RootBlockIo);
                
                if (EFI_ERROR(Status) || RootBlockIo == NULL)
                    continue;
                
                /* For CD-ROM: match BlockSize=2048 and RemovableMedia */
                if (BlockIo->Media->BlockSize == 2048 && BlockIo->Media->RemovableMedia)
                {
                    if (RootBlockIo->Media->BlockSize == 2048 && 
                        RootBlockIo->Media->RemovableMedia &&
                        !RootBlockIo->Media->LogicalPartition)
                    {
                        PublicBootArcDisk = i;
                        InternalUefiDisk[i].IsThisTheBootDrive = TRUE;
                        FoundBootDevice = TRUE;
                        TRACE("Found CD-ROM boot device at ARC drive index %lu\n", i);
                        break;
                    }
                }
                /* For hard disk partitions: the root device should be before the partition handle */
                else if (InternalUefiDisk[i].UefiHandleIndex < UefiBootRootIndex)
                {
                    /* Check if this root device is likely the parent */
                    /* UEFI typically orders handles: root device, then its partitions */
                    if (RootBlockIo->Media->BlockSize == BlockIo->Media->BlockSize &&
                        !RootBlockIo->Media->LogicalPartition)
                    {
                        /* This might be the parent, but we need to be more certain */
                        /* For now, use the last root device before the boot handle */
                        PublicBootArcDisk = i;
                        InternalUefiDisk[i].IsThisTheBootDrive = TRUE;
                        FoundBootDevice = TRUE;
                        TRACE("Found potential hard disk boot device at ARC drive index %lu\n", i);
                    }
                }
            }
            
            if (!FoundBootDevice && PcBiosDiskCount > 0)
            {
                /* Fallback: use first drive */
                PublicBootArcDisk = 0;
                InternalUefiDisk[0].IsThisTheBootDrive = TRUE;
                TRACE("Could not determine boot device, assuming first drive\n");
            }
        }
    }

    /* PcBiosDiskCount is already set correctly during the loop */
    TRACE("Found %lu root block devices\n", PcBiosDiskCount);
}

static
BOOLEAN
UefiSetBootpath(VOID)
{
    EFI_BLOCK_IO* BootBlockIo = NULL;
    EFI_BLOCK_IO* RootBlockIo = NULL;
    EFI_STATUS Status;
    ULONG ArcDriveIndex;

    TRACE("UefiSetBootpath: Setting up boot path\n");
    
    if (UefiBootRootIndex >= HandleCount || handles == NULL)
    {
        ERR("Invalid boot root index\n");
        return FALSE;
    }

    ArcDriveIndex = PublicBootArcDisk;
    if (ArcDriveIndex >= PcBiosDiskCount || InternalUefiDisk == NULL)
    {
        ERR("Invalid boot arc disk index\n");
        return FALSE;
    }

    /* Get Block I/O protocol for the boot handle (might be a partition) */
    Status = GlobalSystemTable->BootServices->HandleProtocol(
        handles[UefiBootRootIndex],
        &BlockIoGuid,
        (VOID**)&BootBlockIo);
    
    if (EFI_ERROR(Status) || BootBlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for boot handle\n");
        return FALSE;
    }

    /* Get Block I/O protocol for the root device */
    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&RootBlockIo);
    
    if (EFI_ERROR(Status) || RootBlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for boot root device\n");
        return FALSE;
    }

    FrldrBootDrive = (FIRST_BIOS_DISK + ArcDriveIndex);
    
    /* Check if booting from CD-ROM by checking the boot handle properties */
    /* CD-ROMs have BlockSize=2048 and RemovableMedia=TRUE */
    if (BootBlockIo->Media->RemovableMedia == TRUE && BootBlockIo->Media->BlockSize == 2048)
    {
        /* Boot Partition 0xFF is the magic value that indicates booting from CD-ROM */
        FrldrBootPartition = 0xFF;
        RtlStringCbPrintfA(FrLdrBootPath, sizeof(FrLdrBootPath),
                           "multi(0)disk(0)cdrom(%u)", ArcDriveIndex);
        TRACE("Boot path set to CD-ROM: %s\n", FrLdrBootPath);
    }
    else
    {
        ULONG BootPartition;
        PARTITION_TABLE_ENTRY PartitionEntry;

        /* This is a hard disk */
        /* If boot handle is a logical partition, we need to determine which partition number */
        if (BootBlockIo->Media->LogicalPartition)
        {
            /* For logical partitions, we need to find the partition number */
            /* This is tricky - we'll use partition 1 as default for now */
            /* TODO: Properly determine partition number from boot handle */
            BootPartition = FIRST_PARTITION;
            TRACE("Boot handle is logical partition, using partition %lu\n", BootPartition);
        }
        else
        {
            /* Boot handle is the root device itself */
            if (!UefiGetBootPartitionEntry(FrldrBootDrive, &PartitionEntry, &BootPartition))
            {
                ERR("Failed to get boot partition entry\n");
                return FALSE;
            }
        }

        RtlStringCbPrintfA(FrLdrBootPath, sizeof(FrLdrBootPath),
                           "multi(0)disk(0)rdisk(%u)partition(%lu)",
                           ArcDriveIndex, BootPartition);
        TRACE("Boot path set to hard disk: %s\n", FrLdrBootPath);
    }

    return TRUE;
}

BOOLEAN
UefiInitializeBootDevices(VOID)
{
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;
    ULONG ArcDriveIndex;
    PMASTER_BOOT_RECORD Mbr;
    PULONG Buffer;
    ULONG Checksum = 0;
    ULONG Signature;
    ULONG i;

    DiskReadBufferSize = EFI_PAGE_SIZE;
    DiskReadBuffer = MmAllocateMemoryWithType(DiskReadBufferSize, LoaderFirmwareTemporary);
    if (DiskReadBuffer == NULL)
    {
        ERR("Failed to allocate disk read buffer\n");
        return FALSE;
    }

    UefiSetupBlockDevices();
    
    if (PcBiosDiskCount == 0)
    {
        ERR("No block devices found\n");
        return FALSE;
    }

    if (!UefiSetBootpath())
    {
        ERR("Failed to set boot path\n");
        return FALSE;
    }

    /* Handle CD-ROM boot device registration */
    ArcDriveIndex = PublicBootArcDisk;
    if (ArcDriveIndex >= PcBiosDiskCount || InternalUefiDisk == NULL)
    {
        ERR("Invalid boot arc disk index\n");
        return FALSE;
    }

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);
    
    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol\n");
        return FALSE;
    }

    if (BlockIo->Media->RemovableMedia == TRUE && BlockIo->Media->BlockSize == 2048)
    {
        /* Read the MBR from CD-ROM (sector 16) */
        if (!MachDiskReadLogicalSectors(FrldrBootDrive, 16ULL, 1, DiskReadBuffer))
        {
            ERR("Reading MBR from CD-ROM failed\n");
            return FALSE;
        }

        Buffer = (ULONG*)DiskReadBuffer;
        Mbr = (PMASTER_BOOT_RECORD)DiskReadBuffer;

        Signature = Mbr->Signature;
        TRACE("CD-ROM Signature: %x\n", Signature);

        /* Calculate the MBR checksum */
        for (i = 0; i < 2048 / sizeof(ULONG); i++)
        {
            Checksum += Buffer[i];
        }
        Checksum = ~Checksum + 1;
        TRACE("CD-ROM Checksum: %x\n", Checksum);

        /* Fill out the ARC disk block */
        AddReactOSArcDiskInfo(FrLdrBootPath, Signature, Checksum, TRUE);

        FsRegisterDevice(FrLdrBootPath, &UefiDiskVtbl);
        TRACE("Registered CD-ROM boot device: 0x%02X\n", (int)FrldrBootDrive);
    }
    
    return TRUE;
}

UCHAR
UefiGetFloppyCount(VOID)
{
    /* No floppy support in UEFI */
    return 0;
}

BOOLEAN
UefiDiskReadLogicalSectors(
    IN UCHAR DriveNumber,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    ULONG ArcDriveIndex;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;
    ULONG BlockSize;

    if (DriveNumber < FIRST_BIOS_DISK)
        return FALSE;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
    
    /* Check if InternalUefiDisk is initialized and drive index is valid */
    if (InternalUefiDisk == NULL)
    {
        ERR("InternalUefiDisk not initialized\n");
        return FALSE;
    }
    
    /* Check array bounds */
    if (ArcDriveIndex >= 32)
    {
        ERR("Drive index out of bounds: %d (ArcDriveIndex=%lu)\n", DriveNumber, ArcDriveIndex);
        return FALSE;
    }
    
    /* Allow access during initialization: check if handle is set up */
    /* During initialization, Handle is set before GetHarddiskInformation is called */
    if (InternalUefiDisk[ArcDriveIndex].Handle == NULL)
    {
        ERR("Invalid drive number: %d (ArcDriveIndex=%lu, PcBiosDiskCount=%lu, Handle=NULL)\n", 
            DriveNumber, ArcDriveIndex, PcBiosDiskCount);
        return FALSE;
    }

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);
    
    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for drive %d\n", DriveNumber);
        return FALSE;
    }

    /* UEFI Spec: Check MediaPresent */
    if (!BlockIo->Media->MediaPresent)
    {
        ERR("Media not present for drive %d\n", DriveNumber);
        return FALSE;
    }

    BlockSize = BlockIo->Media->BlockSize;

    /* UEFI Spec: ReadBlocks returns EFI_STATUS, must check it */
    Status = BlockIo->ReadBlocks(
        BlockIo,
        BlockIo->Media->MediaId,
        SectorNumber,
        SectorCount * BlockSize,
        Buffer);
    
    if (EFI_ERROR(Status))
    {
        ERR("ReadBlocks failed: DriveNumber=%d, SectorNumber=%llu, SectorCount=%lu, Status=0x%lx\n",
            DriveNumber, SectorNumber, SectorCount, (ULONG)Status);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
UefiDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
{
    ULONG ArcDriveIndex;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;

    if (DriveNumber < FIRST_BIOS_DISK)
        return FALSE;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
    
    if (InternalUefiDisk == NULL)
    {
        ERR("InternalUefiDisk not initialized\n");
        return FALSE;
    }
    
    if (ArcDriveIndex >= 32 || InternalUefiDisk[ArcDriveIndex].Handle == NULL)
    {
        ERR("Invalid drive number: %d\n", DriveNumber);
        return FALSE;
    }

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);
    
    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for drive %d\n", DriveNumber);
        return FALSE;
    }

    /* UEFI Spec: Check MediaPresent */
    if (!BlockIo->Media->MediaPresent)
    {
        ERR("Media not present for drive %d\n", DriveNumber);
        return FALSE;
    }

    Geometry->Cylinders = 1; /* Not relevant for UEFI Block I/O protocol */
    Geometry->Heads = 1;     /* Not relevant for UEFI Block I/O protocol */
    Geometry->SectorsPerTrack = (ULONG)(BlockIo->Media->LastBlock + 1);
    Geometry->BytesPerSector = BlockIo->Media->BlockSize;
    Geometry->Sectors = BlockIo->Media->LastBlock + 1;

    return TRUE;
}

ULONG
UefiDiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    ULONG ArcDriveIndex;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;

    if (DriveNumber < FIRST_BIOS_DISK)
        return 0;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
    
    if (InternalUefiDisk == NULL)
    {
        ERR("InternalUefiDisk not initialized\n");
        return 0;
    }
    
    if (ArcDriveIndex >= 32 || InternalUefiDisk[ArcDriveIndex].Handle == NULL)
    {
        ERR("Invalid drive number: %d\n", DriveNumber);
        return 0;
    }

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);
    
    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for drive %d\n", DriveNumber);
        return 0;
    }

    /* UEFI Spec: Check MediaPresent */
    if (!BlockIo->Media->MediaPresent)
    {
        ERR("Media not present for drive %d\n", DriveNumber);
        return 0;
    }

    return (ULONG)(BlockIo->Media->LastBlock + 1);
}