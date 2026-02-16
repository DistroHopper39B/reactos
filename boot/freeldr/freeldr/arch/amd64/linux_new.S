/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Linux boot support for FreeLoader
 * COPYRIGHT:   Copyright 1999-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2026 Sylas Hollander <distrohopper39b.business@gmail.com>
 */
 
#include <asm.inc>
#include <arch/pc/x86common.h>
#include <arch/pc/pcbios.h>

.code64

.align 4
LinuxGDT:
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 00 */
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 08 */
    .word HEX(FFFF), HEX(0000), HEX(9B00), HEX(00AF) /* 10 */
    .word HEX(FFFF), HEX(0000), HEX(9300), HEX(008F) /* 18 */   
     
.align 4
LinuxGDTPtr:
    .word HEX(27) /* Limit */
#ifdef _USE_ML
    .quad LinuxGDT     /* Base Address */
#else
    .quad LinuxGDT, 0  /* Base Address */
#endif

/*
 * VOID __cdecl
 * BootLinuxKernel(
 *      PVOID Addr,                     <rcx>
 *      struct boot_params *BootParams  <rdx>
 * )
 */
 
PUBLIC BootLinuxKernel
BootLinuxKernel:
    cli
    cld
    
    /* Load GDT */
#ifdef _USE_ML
    lgdt fword ptr LinuxGDTPtr
#else
    lgdt cs:[LinuxGDTPtr][rip]
#endif
    
    /* Setup segments */
    mov eax, HEX(18)
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax
    
    /* Jump to 64-bit boot protocol */
    add rcx, HEX(200)
    
    /* Pass in BootParams */
    mov rsi, rdx
    
    /* Jump to Linux kernel */
    jmp rcx
END