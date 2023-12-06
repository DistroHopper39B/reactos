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

#ifdef _USE_ML
.MBDATA SEGMENT PUBLIC 'DATA'
ASSUME nothing
#endif

    /* Align to 32 bits boundary */
    .align 4

Header:
    /* magic */
    .long FREELDR_MAGIC_NUMBER
    /* load_addr */
    .long FREELDR_BASE
    /* entry_point */
    .long AppleTVEntry

#ifdef _USE_ML
.MBDATA ENDS
#endif


.code32
ASSUME ES:NOTHING, FS:NOTHING, GS:NOTHING

AppleTVEntry:
    /* Setup protected mode stack */
    mov esp, dword ptr ds:[stack32]
    /* Set disk to first partition */
    mov dx, HEX(0180)

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

END
