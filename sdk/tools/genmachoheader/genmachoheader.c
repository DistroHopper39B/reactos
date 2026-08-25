/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS genmachoheader
 * FILE:            tools/genmachoheader/genmacho.c
 * PURPOSE:         Generates a static Mach-O header that can be appended to the beginning of a PE file.
 * PROGRAMMER:      Sylas Hollander
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* host_includes */
#include <typedefs.h>
#include <pecoff.h>

/* Mach-O header */
#include "macho.h"

#define PAGE_SIZE 0x1000

#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align) - 1l))
#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

int main(int argc, char *argv[])
{
    FILE    *InputFile, *OutputFile;
    UINT32  InputFileLength;
    PUCHAR  InputFileBuffer;
    UINT16  DosMagicNumber;
    UINT32  PeHdrOffset;
    UINT32  ObjectsWritten;
    PIMAGE_OPTIONAL_HEADER32    PeOptionalHeader;

    PMACHO_HEADER               MachoHeader;
    PMACHO_SEGMENT_COMMAND      MachoSegmentCommand;
    PMACHO_THREAD_COMMAND_I386  MachoThreadCommand;
    UINT32                      MachoHeaderSize;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: genmachoheader [input file] [output file]\n");
        return EINVAL;
    }

    /* Open PE file */
    InputFile = fopen(argv[1], "rb");
    if (!InputFile)
    {
        fprintf(stderr, "Cannot find input file '%s': %s\n",
                argv[1], strerror(errno));
        return errno;
    }

    /* Parse PE file */
    fseek(InputFile, 0, SEEK_END);
    InputFileLength = ftell(InputFile);
    fseek(InputFile, 0, SEEK_SET);
#if 0
    InputFileBuffer = malloc(InputFileLength + 1);
    if (!InputFileBuffer)
    {
        fprintf(stderr, "Could not allocate %d bytes for input file\n", InputFileLength + 1);
        fclose(InputFile);
        return ENOMEM;
    }

    //fread(InputFileBuffer, InputFileLength, 1, InputFile);
    //fclose(InputFile);
#endif
    fread(&DosMagicNumber, sizeof(WORD), 1, InputFile);
    if (DosMagicNumber != IMAGE_DOS_MAGIC)
    {
        fprintf(stderr, "Input file not a valid MZ image. (expected 0x%X, got 0x%X)\n",
                IMAGE_DOS_MAGIC, DosMagicNumber);
        return EINVAL;
    }

    fseek(InputFile, FIELD_OFFSET(IMAGE_DOS_HEADER, e_lfanew), SEEK_SET);
    fread(&PeHdrOffset, sizeof(LONG), 1, InputFile);
    if (PeHdrOffset == 0)
    {
        fprintf(stderr, "No PE header found!\n");
        return EINVAL;
    }

    UINT16 PeMachine;
    fseek(InputFile, PeHdrOffset + sizeof(UINT32), SEEK_SET);
    fread(&PeMachine, sizeof(UINT16), 1, InputFile);

    if (PeMachine != IMAGE_FILE_MACHINE_I386)
    {
        fprintf(stderr, "Only i386 executables are supported at this time.\n");
        return EINVAL;
    }

    UINT16 PeOptionalHeaderSize;
    fseek(InputFile, FIELD_OFFSET(IMAGE_FILE_HEADER, SizeOfOptionalHeader) - sizeof(UINT16), SEEK_CUR);
    fread(&PeOptionalHeaderSize, sizeof(UINT16), 1, InputFile);

    if (!PeOptionalHeaderSize)
    {
        fprintf(stderr, "No optional header found!\n");
        return EINVAL;
    }

    PeOptionalHeader = malloc(sizeof(IMAGE_OPTIONAL_HEADER32));
    if (!PeOptionalHeader)
    {
        fprintf(stderr, "Cannot allocate memory for optional header!\n");
        return ENOMEM;
    }

    fseek(InputFile, sizeof(WORD), SEEK_CUR);
    fread(PeOptionalHeader, sizeof(IMAGE_OPTIONAL_HEADER32), 1, InputFile);

    /* Convert PE executable header to Mach-O */
    MachoHeader = calloc(1, PAGE_SIZE);
    if (!MachoHeader)
    {
        fprintf(stderr, "Failed to allocate memory for Mach-O header!\n");
        return ENOMEM;
    }

    MachoHeaderSize = sizeof(MACHO_HEADER)
                    + sizeof(MACHO_SEGMENT_COMMAND)
                    + sizeof(MACHO_THREAD_COMMAND_I386);

    /* Fill out Mach-O header */
    MachoHeader->MagicNumber    = MACHO_MAGIC;
    MachoHeader->CpuType        = 7; /* x86 */
    MachoHeader->CpuSubtype     = 3; /* all x86 */
    MachoHeader->FileType       = 2; /* kernel (static linked) */
    MachoHeader->NumberOfCmds   = 2;
    MachoHeader->SizeOfCmds     = MachoHeaderSize - sizeof(MACHO_HEADER);
    MachoHeader->Flags          = 1;

    /* Fill out first load command. */
    MachoSegmentCommand = (PMACHO_SEGMENT_COMMAND) ((PUCHAR) MachoHeader
                          + sizeof(MACHO_HEADER));

    MachoSegmentCommand->Command            = MACHO_LC_SEGMENT;
    MachoSegmentCommand->CommandSize        = sizeof(MACHO_SEGMENT_COMMAND);

    strcpy(MachoSegmentCommand->SegmentName, "__PE_FILE__");

    MachoSegmentCommand->VMAddress          = PeOptionalHeader->ImageBase;

    /*
     * SizeOfImage should always be a multiple of SectionAlignment, but it isn't on GCC and
     * boot.efi wants it to also be aligned to the EFI page size (0x1000), plus one so that
     * BootArgs is allocated correctly.
     */
    MachoSegmentCommand->VMSize             = ROUND_UP(PeOptionalHeader->SizeOfImage,
                                              PAGE_SIZE) + 1;

    MachoSegmentCommand->FileOffset         = PAGE_SIZE;
    MachoSegmentCommand->FileSize           = InputFileLength;

    MachoSegmentCommand->MaximumProtection  = 7; /* ??? */
    MachoSegmentCommand->InitialProtection  = 5; /* ??? */

    MachoSegmentCommand->NumberOfSections   = 0;
    MachoSegmentCommand->Flags              = 0;

    MachoThreadCommand = (PMACHO_THREAD_COMMAND_I386)
                         ((PUCHAR) MachoSegmentCommand + sizeof(MACHO_SEGMENT_COMMAND));

    MachoThreadCommand->Command                = MACHO_LC_UNIXTHREAD;
    MachoThreadCommand->CommandSize            = sizeof(MACHO_THREAD_COMMAND_I386);
    MachoThreadCommand->Flavor                 = i386_THREAD_STATE;
    MachoThreadCommand->Count                  = i386_THREAD_STATE_COUNT;

    /* all registers are blank except for EIP, which is the entry point. */
    MachoThreadCommand->State.Eip              = PeOptionalHeader->ImageBase +
                                                 PeOptionalHeader->AddressOfEntryPoint;

    /* Write Mach-O output file */
    OutputFile = fopen(argv[2], "wb");
    if (!OutputFile)
    {
        fprintf(stderr, "Cannot open output file %s: %s\n",
                argv[2], strerror(errno));
        return ENOENT;
    }

    /* Copy the Mach-O header to the beginning of the new file. */
    ObjectsWritten = fwrite(MachoHeader, PAGE_SIZE, 1, OutputFile);
    if (!ObjectsWritten)
    {
        fprintf(stderr, "Cannot write to output file %s: %s\n",
                argv[2], strerror(errno));
        fclose(OutputFile);
        return errno;
    }

    fclose(OutputFile);
    printf("Successfully generated Mach-O header %s from PE image %s\n",
           argv[2], argv[1]);
    return 0;
}
