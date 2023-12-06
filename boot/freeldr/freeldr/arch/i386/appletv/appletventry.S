/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Apple TV entry point
 * COPYRIGHT:   Copyright 2023 DistroHopper39B (distrohopper39b.business@gmail.com)
 */

#include <asm.inc>
#include <arch/pc/x86common.h>
#include <arch/pc/pcbios.h>

#define FREELDR_MAGIC_NUMBER        HEX(F00DBEEF)


#ifdef _USE_ML
EXTERN __bss_start__:DWORD
EXTERN __bss_end__:DWORD
#endif

EXTERN _FrldrStartup:DWORD
EXTERN _AppleTVVideoInit:DWORD

    /* Align to 32 bits boundary */
    .align 4

Header:
    /* magic */
    .long FREELDR_MAGIC_NUMBER
    /* load_addr */
    .long FREELDR_BASE
    /* entry_point */
    .long AppleTVEntry

.code32
ASSUME ES:NOTHING, FS:NOTHING, GS:NOTHING

AppleTVEntry:
    /* Setup protected mode stack */
    mov esp, dword ptr ds:[stack32]
    /* Set disk to first partition */
    mov dx, HEX(0180)
    /* Load the GDT */
#ifdef _USE_ML
    lgdt fword ptr gdtptr
#else
    lgdt gdtptr
#endif
    /* Load the IDT */
#ifdef _USE_ML
    lidt fword ptr ds:[i386idtptr]
#else
    lidt i386idtptr
#endif
    /* Jump to FrldrStartup */
    jmp _FrldrStartup

PUBLIC _AppleTVInfoPtr
_AppleTVInfoPtr:
    
PUBLIC cmdline
cmdline:
    .long 0

stack32:
    .long STACKADDR
    .align 4    /* force 4-byte alignment */

.align 4
gdt:
    /* NULL Descriptor */
   .word HEX(0000)
   .word HEX(0000)
   .word HEX(0000)
   .word HEX(0000)

    /* 32-bit flat CS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9A00)
    .word HEX(00CF)

    /* 32-bit flat DS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9200)
    .word HEX(00CF)

    /* 16-bit real mode CS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9E00)
    .word HEX(0000)

    /* 16-bit real mode DS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9200)
    .word HEX(0000)

/* GDT table pointer */
gdtptr:
    .word HEX(27)       /* Limit */
    .long gdt           /* Base Address */
END