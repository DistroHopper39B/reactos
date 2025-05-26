/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Drive access routines for the original Apple TV
 * COPYRIGHT:   Copyright 2023 Sylas Hollander (distrohopper39b.business@gmail.com)
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

/* FUNCTIONS *****************************************************************/

static
VOID
AppleTVIdeUpdateProgIf(VOID)
{
    PCI_TYPE1_CFG_BITS  PciCfg1;
    ULONG               PciData;
    UINT8               ClassCode, Subclass, ProgIf, RevId;
    
    /* Select IDE controller */
    PciCfg1.u.bits.Enable           = 1;
    PciCfg1.u.bits.BusNumber        = 0x00;
    PciCfg1.u.bits.DeviceNumber     = 0x1f;
    PciCfg1.u.bits.FunctionNumber   = 0x1;
    
    /* Select register */
    PciCfg1.u.bits.RegisterNumber   = 0x8;
    PciCfg1.u.bits.Reserved         = 0;
    
    WRITE_PORT_ULONG(PCI_TYPE1_ADDRESS_PORT, PciCfg1.u.AsULONG);
    PciData = READ_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT);
    
    ClassCode   = (PciData >> 24) & 0xFF;
    Subclass    = (PciData >> 16) & 0xFF;
    ProgIf      = (PciData >> 8) & 0xFF;
    RevId       = (PciData) & 0xFF;
    
    if (ProgIf == 0x8A)
    {
        // nothing to do
        TRACE("PCI: Nothing to do.\n");
    }
    else if (ProgIf == 0x8F)
    {
        TRACE("PCI: Changing Prog IF to work on Apple TV\n");
        // change value
        ProgIf = 0x8A;
        
        // reconstruct data
        PciData = (UINT32) (ClassCode << 24)
                            | (UINT32) (Subclass << 16)
                            | (UINT32) (ProgIf << 8)
                            | (UINT32) (RevId);
        WRITE_PORT_ULONG(PCI_TYPE1_ADDRESS_PORT, PciCfg1.u.AsULONG);
        WRITE_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT, PciData);
    }
    else
    {
        // Either running in VirtualBox or on a Mac, either way don't stop here; we'll either
        // have no problem or later problems.
        ERR("Unsupported IDE controller!\n");
    }
}

VOID
AppleTVDiskInit(VOID)
{
    UCHAR DetectedCount;
    UCHAR UnitNumber;
    PDEVICE_UNIT DeviceUnit = NULL;

    ASSERT(!AtaInitialized);

    AtaInitialized = TRUE;
    
    // Fixup IDE controller
    AppleTVIdeUpdateProgIf();

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

    TRACE("AppleTVDiskReadLogicalSectors() DriveNumber: 0x%x SectorNumber: %I64d SectorCount: %d Buffer: 0x%x\n",
          DriveNumber, SectorNumber, SectorCount, Buffer);

    DeviceUnit = AppleTVDiskDriveNumberToDeviceUnit(DriveNumber);
    if (!DeviceUnit)
        return FALSE;

    return AtaReadLogicalSectors(DeviceUnit, SectorNumber, SectorCount, Buffer);
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

static LONG lReportError = 0; // >= 0: display errors; < 0: hide errors.

LONG DiskReportError(BOOLEAN bShowError)
{
    /* Set the reference count */
    if (bShowError) ++lReportError;
    else            --lReportError;
    return lReportError;
}