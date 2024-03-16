/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS pe2macho
 * FILE:            tools/pe2macho/pe2macho.c
 * PURPOSE:         Converts a static PE file to a static Mach-O file
 * PROGRAMMER:      DistroHopper39B
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// host_includes
#include <typedefs.h>
#include <pecoff.h>

// Mach-O header
#include "macho.h"

#ifdef _MSC_VER
#define __builtin_ctz(x) __tzcnt_u32(x)
#endif

static
PIMAGE_FILE_HEADER
FindFileHeaderFromDosHeader(PIMAGE_DOS_HEADER PeBase)
{
    PIMAGE_FILE_HEADER  FileHeader;
    PUINT               Signature;
    
    Signature = (PUINT) (((PUCHAR) PeBase) + PeBase->e_lfanew);
    
    if (*Signature != IMAGE_NT_SIGNATURE)
    {
        return NULL;
    }
    
    FileHeader = (PIMAGE_FILE_HEADER) (Signature + 1);
    return FileHeader;
}

static
PIMAGE_OPTIONAL_HEADER32
FindOptionalHeaderFromFileHeader(PIMAGE_FILE_HEADER FileHeader)
{
    PIMAGE_OPTIONAL_HEADER32    OptionalHeader;
    
    OptionalHeader = (PIMAGE_OPTIONAL_HEADER32) (((PUCHAR) FileHeader) + sizeof(IMAGE_FILE_HEADER));
    if (OptionalHeader->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        return NULL;
    }
    return OptionalHeader;
}

static
PMACHO_HEADER
CreateMachOHeaderFromPeHeader(PIMAGE_OPTIONAL_HEADER32 OptionalHeader, PUINT Size)
{
    UINT32                  MachoInfoSize;
    PMACHO_HEADER           MachoHeader;
    PMACHO_SEGMENT_COMMAND  MachoSegmentCommand;
    PMACHO_SECTION          MachoSection;
    UINT32                  SizeOfExecData;
    
    // HACK ALERT!!!
    // PE executables can have the entry point anywhere within the executable's .text segment but static Mach-O executables expect it to be at __TEXT,__text all the time. We are going
    // to work around this by making the executable have one massive __TEXT,__text segment starting at PE ImageBase + EntryPoint and going to the end of the file.
    
    MachoInfoSize = sizeof(MACHO_HEADER) + sizeof(MACHO_SEGMENT_COMMAND) + sizeof(MACHO_SECTION);
    MachoHeader = malloc(MachoInfoSize);
    if (!MachoHeader)
    {
        fprintf(stderr, "Could not allocate %d bytes for Mach-O info\n", MachoInfoSize);
        return NULL;
    }
    
    SizeOfExecData = OptionalHeader->SizeOfInitializedData - 1;
    
    // Fill out Mach-O header.
    MachoHeader->MagicNumber    = MACHO_MAGIC;
    
    MachoHeader->CpuType        = 7; // x86
    MachoHeader->CpuSubtype     = 3; // all x86
    
    MachoHeader->FileType       = 2; // kernel (static linked)
    
    MachoHeader->NumberOfCmds   = 1; // HACK ALERT!!!
    MachoHeader->SizeOfCmds     = sizeof(MACHO_SEGMENT_COMMAND) + sizeof(MACHO_SECTION);
    
    MachoHeader->Flags          = 1;
    
    // Fill out first and only load command.
    MachoSegmentCommand = (PMACHO_SEGMENT_COMMAND) (((PUCHAR) MachoHeader) + sizeof(MACHO_HEADER));
    
    MachoSegmentCommand->Command            = MACHO_LC_SEGMENT;
    MachoSegmentCommand->CommandSize        = sizeof(MACHO_SEGMENT_COMMAND) + sizeof(MACHO_SECTION);
    
    strcpy(MachoSegmentCommand->SegmentName, "__TEXT");
    
    MachoSegmentCommand->VMAddress          = OptionalHeader->ImageBase;
    MachoSegmentCommand->VMSize             = SizeOfExecData;
    
    MachoSegmentCommand->FileOffset         = 0;
    MachoSegmentCommand->FileSize           = SizeOfExecData;
    
    MachoSegmentCommand->MaximumProtection  = 7; // ???
    MachoSegmentCommand->InitialProtection  = 5; // ???
    
    MachoSegmentCommand->NumberOfSections   = 1;
    MachoSegmentCommand->Flags              = 0;
    
    // Fill out first and only section.
    MachoSection = (PMACHO_SECTION) (((PUCHAR) MachoSegmentCommand) + sizeof(MACHO_SEGMENT_COMMAND));
    
    strcpy(MachoSection->SectionName, "__text");
    strcpy(MachoSection->SegmentName, "__TEXT");
    
    MachoSection->Address               = OptionalHeader->ImageBase + OptionalHeader->AddressOfEntryPoint; // Entry address!!
    MachoSection->Size                  = SizeOfExecData - (OptionalHeader->AddressOfEntryPoint + OptionalHeader->SectionAlignment);
    MachoSection->Offset                = OptionalHeader->AddressOfEntryPoint;
    
    MachoSection->Alignment             = __builtin_ctz(OptionalHeader->SectionAlignment);
    
    MachoSection->RelocationOffset      = 0; // no relocations here :D
    MachoSection->NumberOfRelocation    = 0;
    
    MachoSection->Flags                 = 0x80000400;
    
    MachoSection->Reserved1             = 0;
    MachoSection->Reserved2             = 0;
    
    *Size = MachoInfoSize;
    
    return MachoHeader;
}

INT
main(INT argc, PCHAR argv[])
{
    FILE    *InputFile, *OutputFile;
    UINT32  FileLength;
    PUCHAR  FileBuffer;
    UINT32  ObjectsWritten;
    
    PIMAGE_FILE_HEADER          PeFileHeader;
    PIMAGE_OPTIONAL_HEADER32    PeOptionalHeader;
    
    PMACHO_HEADER               MachoHeader;
    UINT32                      MachoSize;
    
    
    // Check arguments
    if (argc != 3)
    {
        fprintf(stderr, "Usage: pe2macho [input file] [output file]\n");
        return 1;
    }
    
    // Open file
    InputFile = fopen(argv[1], "rb");
    if (!InputFile)
    {
        fprintf(stderr, "Cannot find input file!\n");
        return 2;
    }
    
    // Find end of file
    fseek(InputFile, 0, SEEK_END);
    FileLength = ftell(InputFile);
    fseek(InputFile, 0, SEEK_SET);
    
    FileBuffer = malloc(FileLength + 1);
    if (!FileBuffer)
    {
        fprintf(stderr, "Could not allocate %d bytes for input file\n", FileLength + 1);
        fclose(InputFile);
        return 3;
    }
    
    // Read contents of file into buffer and then close file
    fread(FileBuffer, FileLength, 1, InputFile);
    fclose(InputFile);
    
    // Find DOS header
    if (((PIMAGE_DOS_HEADER) FileBuffer)->e_magic != IMAGE_DOS_MAGIC)
    {
        fprintf(stderr, "Input file not a valid MZ image. (expected 0x%X, got 0x%X)\n", IMAGE_DOS_MAGIC, ((PIMAGE_DOS_HEADER)FileBuffer)->e_magic);
        return 4;
    }
    
    // Find PE/COFF file header
    PeFileHeader = FindFileHeaderFromDosHeader((PIMAGE_DOS_HEADER) FileBuffer);
    if (!PeFileHeader)
    {
        fprintf(stderr, "Input file not a valid PE/COFF image!\n");
        return 5;
    }
    
    // Check arch
    if (PeFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
    {
        fprintf(stderr, "Only 32 bit x86 executables are supported at this time.\n");
        return 6;
    }
    
    // Make sure there's an optional header
    if (!PeFileHeader->SizeOfOptionalHeader)
    {
        fprintf(stderr, "No optional header found!\n");
        return 7;
    }
    
    // Find optional header
    PeOptionalHeader = FindOptionalHeaderFromFileHeader(PeFileHeader);
    if (!PeOptionalHeader)
    {
        fprintf(stderr, "Invalid optional header!\n");
        return 8;
    }
    
    // Convert PE executable header to Mach-O
    MachoHeader = CreateMachOHeaderFromPeHeader(PeOptionalHeader, &MachoSize);
    if (!MachoHeader)
    {
        fprintf(stderr, "Failed to create Mach-O header!\n");
        return 9;
    }
    
    // Remove PE header
    memset(FileBuffer, 0, PeOptionalHeader->SizeOfHeaders);
    
    // Copy Mach-O header to top of buffer
    memcpy(FileBuffer, MachoHeader, MachoSize);
    
    // Open output file
    OutputFile = fopen(argv[2], "wb");
    if (!OutputFile)
    {
        fprintf(stderr, "Cannot open output file: %s\n", argv[2]);
        return 10;
    }
    
    ObjectsWritten = fwrite(FileBuffer, FileLength, 1, OutputFile);
    if (!ObjectsWritten)
    {
        fprintf(stderr, "Cannot write to output file: %s\n", argv[2]);
        fclose(OutputFile);
        return 11;
    }
    
    fclose(OutputFile);
    printf("Successfully converted PE image %s to Mach-O image %s\n", argv[1], argv[2]);    
    return 0;
}