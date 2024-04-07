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
#define MACHO_LC_UNIXTHREAD 0x5 /* UNIX thread load command */

#define i386_THREAD_STATE 1

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
    UINT32 Eax;
    UINT32 Ebx;
    UINT32 Ecx;
    UINT32 Edx;
    UINT32 Edi;
    UINT32 Esi;
    UINT32 Ebp;
    UINT32 Esp;
    UINT32 Ss;
    UINT32 Eflags;
    UINT32 Eip;
    UINT32 Cs;
    UINT32 Ds;
    UINT32 Es;
    UINT32 Fs;
    UINT32 Gs;
} MACHO_THREAD_STATE_32;

typedef struct {
    UINT32 Command; /* LC_UNIXTHREAD */
    UINT32 CommandSize; /* Size of segment command */
    UINT32 Flavor; /* Architecture of thread state */
    UINT32 Count; /* Size of saved state */
    
    MACHO_THREAD_STATE_32 State; /* State */
} MACHO_THREAD_COMMAND_X86, *PMACHO_THREAD_COMMAND_X86;

#define i386_THREAD_STATE_COUNT	16

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
