/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS pe2macho
 * FILE:            tools/pe2macho/macho.h
 * PURPOSE:         Mach-O executable format
 * PROGRAMMER:      DistroHopper39B
 */

#pragma once

#define MACHO_MAGIC 0xFEEDFACE /* Mach-O Magic number */
#define MACHO_CIGAM 0xCEFAEDFE /* Big-endian representation */
#define MACHO_OBJECT 0x1 /* Relocatable object file */

#define MACHO_LC_SEGMENT 0x1 /* Segment to be mapped */

/* Mach-O header */
typedef struct {
    UINT32 MagicNumber; /* Mach-O magic number. */

    INT32 CpuType; /* CPU type */
    INT32 CpuSubtype; /* CPU subtype */

    UINT32 FileType; /* Type of Mach-O file */

    UINT32 NumberOfCmds; /* Number of load commands */
    UINT32 SizeOfCmds; /* Size of all load commands */

    UINT32 Flags; /* Executable flags */
} MACHO_HEADER, *PMACHO_HEADER;

typedef struct {
    UINT32 Command; /* LC_SEGMENT */
    UINT32 CommandSize; /* Size of segment command */

    char SegmentName[16]; /* Name of segment */

    UINT32 VMAddress; /* Virtual memory address of this segment */
    UINT32 VMSize; /* Virtual memory size of this segment */

    UINT32 FileOffset; /* File offset of this segment */
    UINT32 FileSize; /* Amount to map from the file */

    INT32 MaximumProtection; /* Maximum virtual memory protection */
    INT32 InitialProtection; /* Initial virtual memory protection */

    UINT32 NumberOfSections; /* Number of sections in this segment */

    UINT32 Flags; /* Segment flags */
} MACHO_SEGMENT_COMMAND, *PMACHO_SEGMENT_COMMAND;

typedef struct {
    char SectionName[16]; /* Name of this section */
    char SegmentName[16]; /* Segment this section goes in */

    UINT32 Address; /* Memory address of this section */
    UINT32 Size; /* Size of this section in bytes */
    UINT32 Offset; /* File offset for this section */

    UINT32 Alignment; /* Alignment of section */

    UINT32 RelocationOffset; /* File offset of relocation entries */
    UINT32 NumberOfRelocation; /* Number of relocation entries */

    UINT32 Flags; /* Section flags */

    UINT32 Reserved1;
    UINT32 Reserved2;
} MACHO_SECTION, *PMACHO_SECTION;