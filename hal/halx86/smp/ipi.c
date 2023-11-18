/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Source file for Inter-Processor Interrupts management
 * COPYRIGHT:   Copyright 2023 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include <smp.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

VOID
NTAPI
HalRequestIpi(
    _In_ KAFFINITY TargetProcessors)
{
    HalpRequestIpi(TargetProcessors);
}
