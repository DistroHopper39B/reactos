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

.code32

.align 4
LinuxGDT:
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 00 */
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 08 */
    .word HEX(FFFF), HEX(0000), HEX(9B00), HEX(00AF) /* 10 */
    .word HEX(FFFF), HEX(0000), HEX(9300), HEX(008F) /* 18 */
       
.align 4
LinuxGDTPtr:
    .word HEX(27) /* Limit */
    .long LinuxGDT, 0

/*
 * VOID __cdecl
 * BootLinuxKernel(
 *      PVOID Addr,                     <esp + 4>
 *      struct boot_params *BootParams  <esp + 8>
 * )
 */

PUBLIC _BootLinuxKernel
_BootLinuxKernel:
    cli
    cld
    
    /* Load GDT */
#ifdef _USE_ML
    lgdt fword ptr cs:[LinuxGDTPtr]
#else
    lgdt cs:[LinuxGDTPtr]
#endif

    /* Setup segments */
    mov eax, HEX(18)
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax
    
    /* Clear regs */
    xor ebp, ebp
    xor edi, edi
    xor ebx, ebx
    
    /* Pass in BootParams */
    mov esi, dword ptr [esp + 8]
    
    /* Jump to Linux kernel */
    jmp dword ptr [esp + 4]    
END