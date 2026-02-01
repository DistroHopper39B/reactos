/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Drive access routines for the original Apple TV
 * COPYRIGHT:   Authors of xboxdisk.c
 *              Copyright 2023-2026 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>
#include <hwide.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(DISK);

/* GLOBALS *******************************************************************/

static PDEVICE_UNIT HardDrive = NULL;
static PDEVICE_UNIT CdDrive = NULL;
static BOOLEAN AtaInitialized = FALSE;

UCHAR FrldrBootDrive = 0x80; // Drive 1
ULONG FrldrBootPartition = 1; // Partition 1

/* DISK IO ERROR SUPPORT *****************************************************/

static LONG lReportError = 0; // >= 0: display errors; < 0: hide errors.

LONG DiskReportError(BOOLEAN bShowError)
{
    /* Set the reference count */
    if (bShowError) ++lReportError;
    else            --lReportError;
    return lReportError;
}

#if 0 // TODO: ATA/IDE error code descriptions.
static PCSTR DiskGetErrorCodeString(ULONG ErrorCode)
{
    switch (ErrorCode)
    {
    default: return "unknown error code";
    }
}
#endif

static VOID DiskError(PCSTR ErrorString, ULONG ErrorCode)
{
    CHAR ErrorCodeString[200];

    if (lReportError < 0)
        return;

#if 0 // TODO: ATA/IDE error code descriptions.
    sprintf(ErrorCodeString, "%s\n\nError Code: 0x%lx\nError: %s",
            ErrorString, ErrorCode, DiskGetErrorCodeString(ErrorCode));
#else
    UNREFERENCED_PARAMETER(ErrorCode);
    sprintf(ErrorCodeString, "%s", ErrorString);
#endif

    ERR("%s\n", ErrorCodeString);
    UiMessageBox(ErrorCodeString);
}

/* FUNCTIONS *****************************************************************/

CONFIGURATION_TYPE
DiskGetConfigType(
    _In_ UCHAR DriveNumber)
{
    if ((DriveNumber == FrldrBootDrive)/* && DiskIsDriveRemovable(DriveNumber) */ && (FrldrBootPartition == 0xFF))
        return CdromController; /* This is our El Torito boot CD-ROM */
    else
        return DiskPeripheral;
}

VOID
AppleTVDiskInit(VOID)
{
    UCHAR DetectedCount;
    UCHAR UnitNumber;
    PDEVICE_UNIT DeviceUnit = NULL;

    ASSERT(!AtaInitialized);

    AtaInitialized = TRUE;

    /* Find first HDD and CD */
    AtaInit(&DetectedCount);
    for (UnitNumber = 0; UnitNumber <= DetectedCount; UnitNumber++)
    {
        DeviceUnit = AtaGetDevice(UnitNumber);
        if (DeviceUnit)
        {
            if (DeviceUnit->Flags & ATA_DEVICE_ATAPI)
            {
                if (!CdDrive)
                    CdDrive = DeviceUnit; // should not happen on real apple tv
            }
            else
            {
                if (!HardDrive)
                    HardDrive = DeviceUnit;
            }
        }
    }
}

static inline
PDEVICE_UNIT
AppleTVDiskDriveNumberToDeviceUnit(UCHAR DriveNumber)
{
    /* AppleTV has only 1 IDE controller and no floppy */
    if (DriveNumber < 0x80 || (DriveNumber & 0x0F) >= 2)
        return NULL;

    if (!AtaInitialized)
        AppleTVDiskInit();

    /* HDD */
    if ((DriveNumber == 0x80) && HardDrive)
        return HardDrive;
    /* CD */
    if (((DriveNumber & 0xF0) > 0x80) && CdDrive)
        return CdDrive;


    return NULL;
}

BOOLEAN
AppleTVDiskReadLogicalSectors(
    IN UCHAR DriveNumber,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    PDEVICE_UNIT DeviceUnit;
    BOOLEAN Success;

    TRACE("AppleTVDiskReadLogicalSectors() DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d Buffer: 0x%x\n",
          DriveNumber, SectorNumber, SectorCount, Buffer);

    DeviceUnit = AppleTVDiskDriveNumberToDeviceUnit(DriveNumber);
    if (!DeviceUnit)
        return FALSE;

    Success = AtaReadLogicalSectors(DeviceUnit, SectorNumber, SectorCount, Buffer);
    if (!Success)
        DiskError("Disk Read Failed", -1);
    return Success;
}

BOOLEAN
AppleTVDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
{
    PDEVICE_UNIT DeviceUnit;

    TRACE("AppleTVDiskGetDriveGeometry(0x%x)\n", DriveNumber);

    DeviceUnit = AppleTVDiskDriveNumberToDeviceUnit(DriveNumber);
    if (!DeviceUnit)
        return FALSE;

    Geometry->Cylinders = DeviceUnit->Cylinders;
    Geometry->Heads = DeviceUnit->Heads;
    Geometry->Sectors = DeviceUnit->SectorsPerTrack;
    Geometry->BytesPerSector = DeviceUnit->SectorSize;

    return TRUE;
}

ULONG
AppleTVDiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    PDEVICE_UNIT DeviceUnit;

    DeviceUnit = AppleTVDiskDriveNumberToDeviceUnit(DriveNumber);
    if (!DeviceUnit)
        return 1; // Unknown count.

    /*
     * If LBA is supported then the block size will be 64 sectors (32k).
     * If not then the block size is the size of one track.
     */
    if (DeviceUnit->Flags & ATA_DEVICE_LBA)
        return 64;
    else
        return DeviceUnit->SectorsPerTrack;
}

UCHAR
AppleTVGetFloppyCount(VOID)
{
    /* No floppy drive present */
    return 0;
}

UCHAR
DriveMapGetBiosDriveNumber(PCSTR DeviceName)
{
    return 0;
}

BOOLEAN PxeInit(VOID)
{
    return FALSE;
}