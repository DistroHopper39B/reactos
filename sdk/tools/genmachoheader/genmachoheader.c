/*
 * PROJECT:     genmachoheader
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Generates a static Mach-O header that can be appended to the beginning of a PE file
 * COPYRIGHT:   Copyright 2024-2026 Sylas Hollander <distrohopper39b.business@gmail.com>
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

typedef struct _INTERNAL_PECTX
{
    WORD    DosMagicNumber;
    LONG    HeaderOffset;
    WORD    Machine;
    SHORT   OptionalHeaderSize;
    ULONG   ImageBase;
    ULONG   EntryPoint;
    ULONG   SizeOfImage;
} INTERNAL_PECTX, *PINTERNAL_PECTX;

#define PAGE_SIZE 0x1000

#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align) - 1l))
#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

static
BOOLEAN
ParsePeFile(FILE *PeFile, PINTERNAL_PECTX PeCtx)
{
    IMAGE_OPTIONAL_HEADER32 PeOptionalHeader = {0};

    /* DOS Magic Number */
    if (fseek(PeFile, 0, SEEK_SET))
    {
        perror("Cannot go to the beginning of the file");
        return FALSE;
    }

    if (!fread(&PeCtx->DosMagicNumber, sizeof(WORD), 1, PeFile))
    {
        fprintf(stderr, "Cannot read DOS magic number\n");
        return FALSE;
    }

    if (PeCtx->DosMagicNumber != IMAGE_DOS_MAGIC)
    {
        fprintf(stderr, "Input file not a valid MZ image. (expected 0x%X, got 0x%X)\n",
                IMAGE_DOS_MAGIC, PeCtx->DosMagicNumber);
        return FALSE;
    }

    /* PE header offset */
    if (fseek(PeFile, FIELD_OFFSET(IMAGE_DOS_HEADER, e_lfanew), SEEK_SET))
    {
        perror("Cannot find PE header offset");
        return FALSE;
    }

    if (!fread(&PeCtx->HeaderOffset, sizeof(LONG), 1, PeFile))
    {
        fprintf(stderr, "Cannot read PE header offset\n");
        return FALSE;
    }

    if (PeCtx->HeaderOffset == 0)
    {
        fprintf(stderr, "No PE header found!\n");
        return FALSE;
    }

    /* PE Architecture (Machine) */
    if (fseek(PeFile, PeCtx->HeaderOffset + sizeof(UINT32), SEEK_SET))
    {
        perror("Cannot find PE architecture");
        return FALSE;
    }

    if (!fread(&PeCtx->Machine, sizeof(WORD), 1, PeFile))
    {
        fprintf(stderr, "Cannot read PE architecture\n");
        return FALSE;
    }

    if (PeCtx->Machine != IMAGE_FILE_MACHINE_I386)
    {
        fprintf(stderr, "Only i386 executables are supported at this time.\n");
        return FALSE;
    }

    /* SizeOfOptionalHeader */
    if (fseek(PeFile,
              FIELD_OFFSET(IMAGE_FILE_HEADER, SizeOfOptionalHeader) - sizeof(SHORT),
              SEEK_CUR))
    {
        perror("Cannot find optional header size");
        return FALSE;
    }

    if (!fread(&PeCtx->OptionalHeaderSize, sizeof(SHORT), 1, PeFile))
    {
        fprintf(stderr, "Cannot read optional header size\n");
        return FALSE;
    }

    if (!PeCtx->OptionalHeaderSize)
    {
        fprintf(stderr, "No optional header found!\n");
        return FALSE;
    }

    /* Optional header (we do most things with this) */
    if (fseek(PeFile, sizeof(WORD), SEEK_CUR))
    {
        perror("Cannot find optional header");
        return FALSE;
    }

    if (!fread(&PeOptionalHeader, sizeof(IMAGE_OPTIONAL_HEADER32), 1, PeFile))
    {
        fprintf(stderr, "Cannot read optional header\n");
        return FALSE;
    }

    PeCtx->ImageBase    = PeOptionalHeader.ImageBase;
    PeCtx->EntryPoint   = PeOptionalHeader.AddressOfEntryPoint;
    PeCtx->SizeOfImage  = PeOptionalHeader.SizeOfImage;

    return TRUE;
}

int
main(int argc, char *argv[])
{
    FILE                        *InputFile, *OutputFile;
    PSTR                        InputFileName, OutputFileName, SegmentName;
    SIZE_T                      InputFileLength;

    INTERNAL_PECTX              PeCtx;
    PMACHO_HEADER               MachoHeader;
    PMACHO_SEGMENT_COMMAND      MachoSegmentCommand;
    PMACHO_THREAD_COMMAND_I386  MachoThreadCommand;
    SIZE_T                      MachoHeaderSize;

    if (argc != 4)
    {
        fprintf(stderr, "Usage: genmachoheader [input] [output] [Mach-O segment name]\n");
        return EINVAL;
    }

    InputFileName   = argv[1];
    OutputFileName  = argv[2];
    SegmentName     = argv[3];

    /* Open PE file */
    InputFile = fopen(InputFileName, "rb");
    if (!InputFile)
    {
        fprintf(stderr, "Cannot find input file '%s': %s\n",
                InputFileName, strerror(errno));
        return ENOENT;
    }

    /* Get file length */
    if (fseek(InputFile, 0, SEEK_END))
    {
        perror("Cannot seek to end of file");
        fclose(InputFile);
        return 1;
    }

    InputFileLength = ftell(InputFile);
    if (InputFileLength == 0)
    {
        fprintf(stderr, "%s is an empty file!\n", InputFileName);
        fclose(InputFile);
        return 1;
    }

    /* Parse PE file */
    if (!ParsePeFile(InputFile, &PeCtx))
    {
        fprintf(stderr, "Cannot parse PE file!\n");
        fclose(InputFile);
        return 1;
    }

    fclose(InputFile);

    /* Create Mach-O header */
    MachoHeaderSize = sizeof(MACHO_HEADER)
                    + sizeof(MACHO_SEGMENT_COMMAND)
                    + sizeof(MACHO_THREAD_COMMAND_I386);

    MachoHeader = calloc(1, MachoHeaderSize);
    if (!MachoHeader)
    {
        fprintf(stderr, "Failed to allocate memory for Mach-O header!\n");
        return ENOMEM;
    }

    /* Fill out Mach-O header */
    MachoHeader->MagicNumber    = MACHO_MAGIC;
    MachoHeader->CpuType        = 7; /* x86 */
    MachoHeader->CpuSubtype     = 3; /* all x86 */
    MachoHeader->FileType       = 2; /* kernel (static linked) */
    MachoHeader->NumberOfCmds   = 2;
    MachoHeader->SizeOfCmds     = MachoHeaderSize - sizeof(MACHO_HEADER);
    MachoHeader->Flags          = 1;

    /* Fill out first load command. */
    MachoSegmentCommand = (PMACHO_SEGMENT_COMMAND)((PUCHAR)MachoHeader
                          + sizeof(MACHO_HEADER));

    MachoSegmentCommand->Command            = MACHO_LC_SEGMENT;
    MachoSegmentCommand->CommandSize        = sizeof(MACHO_SEGMENT_COMMAND);

    strncpy(MachoSegmentCommand->SegmentName, SegmentName, 15);

    MachoSegmentCommand->VMAddress          = PeCtx.ImageBase;

    /*
     * SizeOfImage should always be a multiple of SectionAlignment, but it isn't on GCC and
     * boot.efi wants it to also be aligned to the EFI page size (0x1000), plus one so that
     * BootArgs is allocated correctly.
     */
    MachoSegmentCommand->VMSize             = ROUND_UP(PeCtx.SizeOfImage,
                                              PAGE_SIZE) + 1;

    MachoSegmentCommand->FileOffset         = MachoHeaderSize;
    MachoSegmentCommand->FileSize           = InputFileLength;

    MachoSegmentCommand->MaximumProtection  = 0;
    MachoSegmentCommand->InitialProtection  = 0;
    MachoSegmentCommand->NumberOfSections   = 0;
    MachoSegmentCommand->Flags              = 0;

    MachoThreadCommand = (PMACHO_THREAD_COMMAND_I386)((PUCHAR)MachoSegmentCommand +
                         sizeof(MACHO_SEGMENT_COMMAND));

    MachoThreadCommand->Command                = MACHO_LC_UNIXTHREAD;
    MachoThreadCommand->CommandSize            = sizeof(MACHO_THREAD_COMMAND_I386);
    MachoThreadCommand->Flavor                 = i386_THREAD_STATE;
    MachoThreadCommand->Count                  = i386_THREAD_STATE_COUNT;

    /* all registers are blank except for EIP, which is the entry point. */
    MachoThreadCommand->State.Eip              = PeCtx.ImageBase +
                                                 PeCtx.EntryPoint;

    /* Create Mach-O output file */
    OutputFile = fopen(OutputFileName, "wb");
    if (!OutputFile)
    {
        fprintf(stderr, "Cannot open output file %s: %s\n",
                OutputFileName, strerror(errno));
        return ENOENT;
    }

    /* Copy the Mach-O header to the new file. */
    if (!fwrite(MachoHeader, MachoHeaderSize, 1, OutputFile))
    {
        fprintf(stderr, "Cannot write to output file\n");
        return EIO;
    }

    fclose(OutputFile);
    printf("Successfully generated Mach-O header %s from PE image %s\n",
           OutputFileName, InputFileName);
    return 0;
}
