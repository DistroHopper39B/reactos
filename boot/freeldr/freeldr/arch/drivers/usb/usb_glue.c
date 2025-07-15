/*
 * PROJECT:     FreeLoader USB Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Glue to connect libpayload to FreeLoader
 * COPYRIGHT:   Copyright 2025 Sylas Hollander <distrohopper39b.business@gmail.com>
 */
 
 
#include <usb/usb_glue.h>

void __cdecl *malloc(size_t size)
{
    return FrLdrTempAlloc(size, TAG_USB);
}

void __cdecl free(void *mem)
{
    FrLdrTempFree(mem, TAG_USB);
}


void __cdecl *calloc(size_t num, size_t size)
{
    void *ptr = malloc(size * num);
    RtlZeroMemory(ptr, size);
    return ptr;
}
