/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Xbox (BRFR) partitioning scheme support
 * COPYRIGHT:   Copyright 2004 Gé van Geldorp <gvg@reactos.org>
 *              Copyright 2019 Stanislav Motylkov <x86corez@gmail.com>
 */

#ifndef _M_ARM
// #include <freeldr.h>

/* BRFR signature at disk offset 0x600 (== 3 * 0x200) */
#define XBOX_SIGNATURE_SECTOR 3
#define XBOX_SIGNATURE        ('B' | ('R' << 8) | ('F' << 16) | ('R' << 24))

/* Default hardcoded partition number to boot from Xbox disk */
#define FATX_DATA_PARTITION 1

static struct
{
    ULONG SectorCountBeforePartition;
    ULONG PartitionSectorCount;
    UCHAR SystemIndicator;
} XboxPartitions[] =
{
    /* This is in the \Device\Harddisk0\Partition.. order used by the Xbox kernel */
    { 0x0055F400, 0x0098F800, PARTITION_FAT32  }, /* Store , E: */
    { 0x00465400, 0x000FA000, PARTITION_FAT_16 }, /* System, C: */
    { 0x00000400, 0x00177000, PARTITION_FAT_16 }, /* Cache1, X: */
    { 0x00177400, 0x00177000, PARTITION_FAT_16 }, /* Cache2, Y: */
    { 0x002EE400, 0x00177000, PARTITION_FAT_16 }  /* Cache3, Z: */
};

BOOLEAN
DiskGetBrfrPartitionEntry(
    IN UCHAR DriveNumber,
    IN ULONG PartitionNumber,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
    /*
     * Get partition entry of an Xbox-standard BRFR partitioned disk.
     */
    if (PartitionNumber >= 1 && PartitionNumber <= RTL_NUMBER_OF(XboxPartitions) &&
        MachDiskReadLogicalSectors(DriveNumber, XBOX_SIGNATURE_SECTOR, 1, DiskReadBuffer))
    {
        if (*((PULONG)DiskReadBuffer) != XBOX_SIGNATURE)
        {
            /* No magic Xbox partitions */
            return FALSE;
        }

        RtlZeroMemory(PartitionTableEntry, sizeof(PARTITION_TABLE_ENTRY));
        PartitionTableEntry->SystemIndicator = XboxPartitions[PartitionNumber - 1].SystemIndicator;
        PartitionTableEntry->SectorCountBeforePartition = XboxPartitions[PartitionNumber - 1].SectorCountBeforePartition;
        PartitionTableEntry->PartitionSectorCount = XboxPartitions[PartitionNumber - 1].PartitionSectorCount;
        return TRUE;
    }

    /* Partition does not exist */
    return FALSE;
}

#endif
