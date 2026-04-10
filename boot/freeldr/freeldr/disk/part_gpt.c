/*
 * PROJECT:     FreeLoader
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     GPT partitioning scheme support
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#ifndef _M_ARM
// #include <freeldr.h>
#include "part_gpt.h"

// #include <debug.h>
// DBG_DEFAULT_CHANNEL(DISK);


/* FUNCTIONS *****************************************************************/

//static // Used also in uefidisk.c
BOOLEAN
DiskReadGptHeader(
    _In_ UCHAR DriveNumber,
    _Out_ PGPT_TABLE_HEADER GptHeader)
{
    /* Read GPT header (1 sector) from LBA 1 */
    if (!MachDiskReadLogicalSectors(DriveNumber, EFI_HEADER_LOCATION, 1, DiskReadBuffer))
        return FALSE;
    RtlCopyMemory(GptHeader, DiskReadBuffer, sizeof(*GptHeader));

    /* Verify GPT signature */
    if (!RtlEqualMemory(GptHeader->Signature, EFI_PARTITION_HEADER_SIGNATURE, 8))
        return FALSE;

    /* Verify revision */
    if (GptHeader->Revision != EFI_TABLE_REVISION)
    {
        TRACE("GPT header has unsupported revision: 0x%x\n", GptHeader->Revision);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
DiskGetGptPartitionEntry(
    _In_ UCHAR DriveNumber,
    _In_ ULONG PartitionNumber,
    _Out_ PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
    GPT_TABLE_HEADER GptHeader;
    GPT_PARTITION_ENTRY GptEntry;
    GEOMETRY Geometry;
    ULONG BlockSize;
    ULONGLONG EntryLba;
    ULONG EntryOffset;
    ULONG EntriesPerBlock;
    GUID UnusedGuid = EFI_PART_TYPE_UNUSED_GUID;

    if (!MachDiskGetDriveGeometry(DriveNumber, &Geometry))
        return FALSE;
    BlockSize = Geometry.BytesPerSector;

    /* Read GPT header */
    if (!DiskReadGptHeader(DriveNumber, &GptHeader))
        return FALSE;

    /* Validate partition number */
    if (PartitionNumber == 0 || PartitionNumber > GptHeader.NumberOfPartitionEntries)
        return FALSE;

    /* Convert to 0-based index */
    ULONG EntryIndex = PartitionNumber - 1;

    EntriesPerBlock = BlockSize / GptHeader.SizeOfPartitionEntry;
    EntryLba = GptHeader.PartitionEntryLba + (EntryIndex / EntriesPerBlock);
    EntryOffset = (EntryIndex % EntriesPerBlock) * GptHeader.SizeOfPartitionEntry;

    /* Read the block containing the partition entry */
    if (!MachDiskReadLogicalSectors(DriveNumber, EntryLba, 1, DiskReadBuffer))
        return FALSE;

    /* Extract partition entry */
    RtlCopyMemory(&GptEntry, (PUCHAR)DiskReadBuffer + EntryOffset, sizeof(GptEntry));

    /* Check if partition is unused */
    if (RtlEqualMemory(&GptEntry.PartitionTypeGuid, &UnusedGuid, sizeof(UnusedGuid)))
        return FALSE;

    /* Convert GPT entry to MBR-style PARTITION_TABLE_ENTRY */
    RtlZeroMemory(PartitionTableEntry, sizeof(*PartitionTableEntry));

    /* Calculate sector offset and count.
     * GPT uses LBA, convert to 512-byte sectors. */
    ULONGLONG SectorCount = (GptEntry.EndingLba - GptEntry.StartingLba + 1);

    /* For GPT, we need to convert from device block size to 512-byte sectors */
    ULONGLONG StartSector = (GptEntry.StartingLba * BlockSize) / 512;
    ULONGLONG SectorCount512 = (SectorCount * BlockSize) / 512;

    PartitionTableEntry->SectorCountBeforePartition = (ULONG)StartSector;
    PartitionTableEntry->PartitionSectorCount = (ULONG)SectorCount512;
    PartitionTableEntry->SystemIndicator = PARTITION_GPT; /* Mark as GPT partition */

    return TRUE;
}

#endif
