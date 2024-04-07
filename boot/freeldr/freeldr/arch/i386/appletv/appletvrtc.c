/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Real-time clock access routine for the original Apple TV
 * COPYRIGHT:   Copyright 2023 DistroHopper39B (distrohopper39b.business@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

/* GLOBALS *******************************************************************/

#define RTC_REGISTER_A   0x0A
#define RTC_REG_A_UIP    0x80  /* Update In Progress bit */

#define BCD_INT(bcd) (((bcd & 0xf0) >> 4) * 10 + (bcd &0x0f))

/* FUNCTIONS *****************************************************************/

static UCHAR
HalpQueryCMOS(UCHAR Reg)
{
  UCHAR Val;
  Reg |= 0x80;

  WRITE_PORT_UCHAR((PUCHAR)0x70, Reg);
  Val = READ_PORT_UCHAR((PUCHAR)0x71);
  WRITE_PORT_UCHAR((PUCHAR)0x70, 0);

  return(Val);
}

TIMEINFO*
AppleTVGetTime(VOID)
{
    static TIMEINFO TimeInfo;

    while (HalpQueryCMOS (RTC_REGISTER_A) & RTC_REG_A_UIP)
    {
        ;
    }

    TimeInfo.Second = BCD_INT(HalpQueryCMOS(0));
    TimeInfo.Minute = BCD_INT(HalpQueryCMOS(2));
    TimeInfo.Hour = BCD_INT(HalpQueryCMOS(4));
    TimeInfo.Day = BCD_INT(HalpQueryCMOS(7));
    TimeInfo.Month = BCD_INT(HalpQueryCMOS(8));
    TimeInfo.Year = BCD_INT(HalpQueryCMOS(9));
    if (TimeInfo.Year > 80)
        TimeInfo.Year += 1900;
    else
        TimeInfo.Year += 2000;

    return &TimeInfo;
}