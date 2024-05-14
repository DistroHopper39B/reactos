/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Apple TV entry point
 * COPYRIGHT:   Copyright 2023 Sylas Hollander (distrohopper39b.business@gmail.com)
 */

#include <asm.inc>
#include <arch/pc/x86common.h>
#include <arch/pc/pcbios.h>

#ifdef _USE_ML
EXTERN __bss_start__:DWORD
EXTERN __bss_end__:DWORD
#endif

#define APPLETV_COMMAND_LINE_OFFSET 4

/* VOID __cdecl BootMain(IN PCCH CmdLine) */
EXTERN _BootMain

/* Align to 32 bits boundary */
.align 4

.code32
ASSUME ES:NOTHING, FS:NOTHING, GS:NOTHING

/* PMACH_BOOTARGS BootArgs */
PUBLIC _BootArgs
_BootArgs:
    .long 0

PUBLIC _AppleTVEntry
_AppleTVEntry:
    /* Set up stack */
    mov esp, dword ptr ds:[stack32]
    
    /* Test if the bootArgs pointer is valid */
    test eax, eax
    jz InfiniteLoop
    
    /* Store the bootArgs pointer */
    mov dword ptr ds:[_BootArgs], eax
    /* Load GDT */
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
    /* Clean out BSS */
    xor eax, eax
    mov edi, offset __bss_start__
    mov ecx, offset __bss_end__ + 3
    sub ecx, edi
    shr ecx, 2
    rep stosd

    /* Initialize the idt */
    call _InitIdt
    
    /* Set up CmdLine */
    mov eax, dword ptr ds:[(_BootArgs)]
    add eax, APPLETV_COMMAND_LINE_OFFSET
    
    /* GO!! */
    push eax
    call _BootMain
    
    /* If this fails, halt the system */
    jmp InfiniteLoop
    
InfiniteLoop:
    cld
    hlt
    jmp InfiniteLoop

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
    
PUBLIC i386idtptr
i386idtptr:
    .word 255           /* Limit */
    .long _i386Idt      /* Base Address */

END