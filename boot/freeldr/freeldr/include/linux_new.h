/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Header for new Linux boot routines
 * COPYRIGHT:   Copyright 2024 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

/*
 * The Linux kernel boot process is documented at
 * https://www.kernel.org/doc/html/v6.8/arch/x86/boot.html
 */

#pragma once

#include <fs.h>
#include <oslist.h>

/* LINUX SETUP HEADER ********************************************************/

#include <pshpack1.h>
typedef struct
{
    UCHAR   SetupSects;
    USHORT  RootFlags;
    ULONG32 SystemSize;
    USHORT  RamSize;
    USHORT  VideoMode;
    USHORT  RootDevice;
    USHORT  BootFlag;
    USHORT  Jump;
    ULONG32 HeaderSignature; 
    USHORT  BootProtocolVersion;
    ULONG32 RealModeSwitch;
    USHORT  StartSysSeg;
    USHORT  KernelVersion;
    UCHAR   TypeOfLoader;
    UCHAR   LoadFlags;
    USHORT  SetupMoveSize;
    ULONG32 Code32Start;
    ULONG32 RamdiskImage;
    ULONG32 RamdiskSize;
    ULONG32 BootsectKludge;
    USHORT  HeapEnd;
    UCHAR   ExtLoaderVer;
    UCHAR   ExtLoaderType;
    ULONG32 CommandLine;
    ULONG32 MaxInitrdAddr;
    ULONG32 KernelAlignment;
    UCHAR   RelocatableKernel;
    UCHAR   MinimumAlignment;
    USHORT  XLoadFlags;
    ULONG32 CmdlineSize;
    ULONG32 HwSarch;
    ULONG64 SarchData;
    ULONG32 PayloadOffset;
    ULONG32 PayloadLength;
    ULONG64 SetupData;
    ULONG64 PreferredLoadAddr;
    ULONG32 MinimumLinearMemory;
    ULONG32 HandoverOffset;
    ULONG32 KernelInfoOffset;
} __attribute__ ((packed)) LINUX_SETUP_HEADER, *PLINUX_SETUP_HEADER;

/* LINUX BOOT PARAMETERS *****************************************************/

typedef struct
{
    UCHAR   OriginalX;
    UCHAR   OriginalY;
    USHORT  ExtMemoryK;
    USHORT  OrigVideoPage;
    UCHAR   OrigVideoMode;
    UCHAR   OrigVideoCols;
    UCHAR   Flags;
    UCHAR   Unused2;
    USHORT  OrigVideoEgaBx;
    USHORT  Unused3;
    UCHAR   OrigVideoLines;
    UCHAR   OrigVideoIsVGA;
    USHORT  OrigVideoPoints; 
    USHORT  FbWidth;
    USHORT  FbHeight;
    USHORT  FbDepth;
    ULONG32 FbBase;
    ULONG32 FbSize;
    USHORT  ClMagic, ClOffset;
    USHORT  FbLineLength;
    UCHAR   RedSize;
    UCHAR   RedPosition;
    UCHAR   GreenSize;
    UCHAR   GreenPosition;
    UCHAR   BlueSize;
    UCHAR   BluePosition;
    UCHAR   ReservedSize;
    UCHAR   ReservedPosition;
    USHORT  VesaPmSeg;
    USHORT  VesaPmOff;
    USHORT  Pages;
    USHORT  VesaAttributes;
    ULONG32 Capabilities;
    ULONG32 ExtendedFbBase;
    UCHAR   Reserved[2];
} __attribute__ ((packed)) LINUX_SCREEN_INFO, *PLINUX_SCREEN_INFO;

/* Video types */

#define VIDEO_TYPE_MDA          0x10
#define VIDEO_TYPE_CGA          0x11
#define VIDEO_TYPE_EGA_MONO     0x20
#define VIDEO_TYPE_EGA_COLOR    0x21
#define VIDEO_TYPE_VGA_COLOR    0x22
#define VIDEO_TYPE_VLFB         0x23
#define VIDEO_TYPE_PICA_S3      0x30
#define VIDEO_TYPE_MIPS_G364    0x31
#define VIDEO_TYPE_SGI          0x33
#define VIDEO_TYPE_DEC_TGA      0x40
#define VIDEO_TYPE_SUN          0x50
#define VIDEO_TYPE_SUN_PCI      0x51
#define VIDEO_TYPE_POWERMAC     0x60
#define VIDEO_TYPE_EFI          0x70

/* Video flags */
#define VIDEO_FLAGS_NOCURSOR            (1 << 0)

/* Video capabilities */
#define VIDEO_CAPABILITY_SKIP_QUIRKS    (1 << 0)
#define VIDEO_CAPABILITY_64BIT_BASE     (1 << 1)

typedef struct
{
    USHORT  Version;
    USHORT  CSeg;
    ULONG32 Offset;
    USHORT  CSeg16;
    USHORT  DSeg;
    USHORT  Flags;
    USHORT  CSegLen;
    USHORT  CSeg16Len;
    USHORT  DSegLen;
} LINUX_APM_BIOS_INFO, *PLINUX_APM_BIOS_INFO;

typedef struct
{
    ULONG32 Signature;
    ULONG32 Command;
    ULONG32 Event;
    ULONG32 PerfLevel;
} LINUX_IST_INFO, *PLINUX_IST_INFO;

typedef struct
{
    USHORT  Length;
    UCHAR   Table[14];
} LINUX_SYSDESC_TABLE, *PLINUX_SYSDESC_TABLE;

typedef struct 
{
    ULONG32 OfwMagic;
    ULONG32 OfwVersion;
    ULONG32 CifHandler;
    ULONG32 IrqDescTable;
} __attribute__ ((packed)) LINUX_OLPC_OFW_HEADER, *PLINUX_OLPC_OFW_HEADER;

typedef struct
{
    UCHAR   Dummy[128];
} LINUX_EDID_INFO, *PLINUX_EDID_INFO;

typedef struct
{
    ULONG32 EfiLoaderSignature;
    ULONG32 EfiSystemTable;
    ULONG32 EfiMemoryDescriptorSize;
    ULONG32 EfiMemoryDescriptorVersion;
    ULONG32 EfiMemoryMap;
    ULONG32 EfiMemoryMapSize;
    ULONG32 EfiSystemTableHigh;
    ULONG32 EfiMemoryMapHigh;
} LINUX_EFI_INFO, *PLINUX_EFI_INFO;

#define LINUX_E820_MAX  128

typedef struct
{
    ULONG64 Address;
    ULONG64 Size;
    ULONG32 Type;
} __attribute__ ((packed)) LINUX_E820_ENTRY, *PLINUX_E820_ENTRY;

#define LINUX_EDD_MBR_SIG_MAX   16
#define LINUX_EDD_MAX           6

typedef struct
{
    USHORT  Length;
    USHORT  InfoFlags;
    ULONG32 NumDefaultCylinders;
    ULONG32 NumDefaultHeads;
    ULONG32 SectorsPerTrack;
    ULONG64 NumberOfSectors;
    USHORT  BytesPerSector;
    ULONG32 DptePtr;
    USHORT  Key;
    UCHAR   DevicePathInfoLength;
    UCHAR   Reserved2;
    USHORT  Reserved3;
    UCHAR   HostBusType[4];
    UCHAR   InterfaceType[8];
    union
    {
        struct
        {
            USHORT  BaseAddress;
            USHORT  Reserved1;
            ULONG32 Reserved2;
        } __attribute__ ((packed)) Isa;
        struct
        {
            UCHAR   Bus;
            UCHAR   Slot;
            UCHAR   Function;
            UCHAR   Channel;
            ULONG32 Reserved;
        } __attribute__ ((packed)) Pci;
        struct
        {
            ULONG64 Reserved;
        } __attribute__ ((packed)) Ibnd;
        struct
        {
            ULONG64 Reserved;
        } __attribute__ ((packed)) Xprs;
        struct
        {
            ULONG64 Reserved;
        } __attribute__ ((packed)) Htpt;
        struct
        {
            ULONG64 Reserved;
        } __attribute__ ((packed)) Unknown;
    } InterfacePath;
    union
    {
        struct 
        {
            UCHAR   Device;
            UCHAR   Reserved1;
            USHORT  Reserved2;
            ULONG32 Reserved3;
            ULONG64 Reserved4;
        } __attribute__ ((packed)) Ata;
        struct
        {
            UCHAR   Device;
            UCHAR   Lun;
            UCHAR   Reserved1;
            UCHAR   Reserved2;
            ULONG32 Reserved3;
            ULONG64 Reserved4;
        } __attribute__ ((packed)) Atapi;
            struct
        {
            USHORT  Id;
            ULONG64 Lun;
            USHORT  Reserved1;
            ULONG32 Reserved2;
        } __attribute__ ((packed)) Scsi;
            struct
        {
            ULONG64 Serial_number;
            ULONG64 Reserved;
        } __attribute__ ((packed)) Usb;
        struct
        {
            ULONG64 Eui;
            ULONG64 Reserved;
        } __attribute__ ((packed)) I1394;
        struct
        {
            ULONG64 Wwid;
            ULONG64 Lun;
        } __attribute__ ((packed)) Fibre;
        struct
        {
            ULONG64 Identity_tag;
            ULONG64 Reserved;
        } __attribute__ ((packed)) I2o;
        struct
        {
            ULONG32 Array_number;
            ULONG32 Reserved1;
            ULONG64 Reserved2;
        } __attribute__ ((packed)) Raid;
        struct
        {
            UCHAR   Device;
            UCHAR   Reserved1;
            USHORT  Reserved2;
            ULONG32 Reserved3;
            ULONG64 Reserved4;
        } __attribute__ ((packed)) Sata;
        struct
        {
            ULONG64 Reserved1;
            ULONG64 Reserved2;
        } __attribute__ ((packed)) Unknown;
    } DevicePath;
    UCHAR   Reserved4;
    UCHAR   Checksum;
} __attribute__ ((packed)) LINUX_EDD_DEVICE_PARAMS, *PLINUX_EDD_DEVICE_PARAMS;

typedef struct
{
    UCHAR                   Device;
    UCHAR                   Version;
    USHORT                  InterfaceSupport;
    USHORT                  LegacyMaxCylinder;
    UCHAR                   LegacyMaxHead;
    UCHAR                   LegacySectorsPerTrack;
    LINUX_EDD_DEVICE_PARAMS Params;
} __attribute__ ((packed)) LINUX_EDD_INFO, *PLINUX_EDD_INFO;

typedef struct
{
    LINUX_SCREEN_INFO       ScreenInfo;
    LINUX_APM_BIOS_INFO     ApmBiosInfo;
    UCHAR                   __pad2[4];
    ULONG64                 TBootAddr;
    LINUX_IST_INFO          IstInfo;
    ULONG64                 Rsdp;
    UCHAR                   __pad3[8];
    UCHAR                   Hd0Info[16];
    UCHAR                   Hd1Info[16];
    LINUX_SYSDESC_TABLE     SysDescTable;
    LINUX_OLPC_OFW_HEADER   OlpcOfwHeader;
    ULONG32                 ExtRamdiskImage;
    ULONG32                 ExtRamdiskSize;
    ULONG32                 ExtCmdlinePtr;
    UCHAR                   __pad4[116];
    LINUX_EDID_INFO         EdidInfo;
    LINUX_EFI_INFO          EfiInfo;
    ULONG32                 AltMemK;
    ULONG32                 Scratch;
    UCHAR                   E820Entries;
    UCHAR                   EddBufEntries;
    UCHAR                   EddMbrSigBufEntries;
    UCHAR                   KeyboardStatus;
    UCHAR                   SecureBootStatus;
    UCHAR                   __pad5[2];
    UCHAR                   Sentinel;
    UCHAR                   __pad6[1];
    LINUX_SETUP_HEADER      SetupHeader;
    UCHAR                   __pad7[0x290 - 0x1f1 - sizeof(LINUX_SETUP_HEADER)];
    ULONG32                 EddMbrSigBuffer[LINUX_EDD_MBR_SIG_MAX];
    LINUX_E820_ENTRY        E820Table[LINUX_E820_MAX];
    UCHAR                   __pad8[48];
    LINUX_EDD_INFO          EddBuffer[LINUX_EDD_MAX];
    UCHAR                   __pad9[276];
} __attribute__ ((packed)) LINUX_BOOT_PARAMS, *PLINUX_BOOT_PARAMS;

#define COMMAND_LINE_SIZE               2048
#define LINUX_BOOT_MAGIC                0x53726448
#define LINUX_LOADER_TYPE_FREELOADER    0x81
#define LINUX_KERNEL_LOAD_ADDRESS       0x100000

ARC_STATUS LoadAndBootLinux(IN ULONG Argc, IN PCHAR Argv[], IN PCHAR Envp[]);