/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FreeLoader Setup and Configuration F2 menu.
 * COPYRIGHT:   Copyright 1998-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2012 Giannis Adamopoulos <gadamopoulos@reactos.org>
 *              Copyright 2022-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <debug.h> // For DbgParseDebugChannels()

/* GLOBALS ********************************************************************/

static PCSTR OptionsMenuList[] =
{
    "FreeLdr debugging",

    NULL,

#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
    "Edit Boot Command Line (F10)",
#endif
#ifdef HAS_OPTION_MENU_CUSTOM_BOOT
    "Custom Boot",
#endif
#ifdef HAS_OPTION_MENU_REBOOT
    "Reboot",
#endif
};

static PCSTR FrldrDbgMsg =
    "Enable FreeLdr debug channels\n"
    "Acceptable syntax: [level1]#channel1[,[level2]#channel2]\n"
    "level can be one of: trace,warn,fixme,err\n"
    "  if the level is omitted all levels\n"
    "  are enabled for the specified channel\n"
    "# can be either + or -\n"
    "channel can be one of the following:\n"
    "  all,warning,memory,filesystem,inifile,ui,disk,cache,registry,\n"
    "  reactos,linux,hwdetect,windows,peloader,scsiport,heap\n"
    "Examples:\n"
    "  trace+windows,trace+reactos\n"
    "  +hwdetect,err-disk\n"
    "  +peloader\n"
    "NOTE: all letters must be lowercase, no spaces allowed.";

/* FUNCTIONS ******************************************************************/

VOID
FreeLdrSetupMenu(
    _In_opt_ OperatingSystemItem* OperatingSystem)
{
    ULONG SelectedMenuItem = 0;

doMenu:
    /* Clear the backdrop */
    UiDrawBackdrop(UiGetScreenHeight());

    if (!UiDisplayMenu(VERSION " Setup and Configuration",
                       OperatingSystem ? NULL : "Press ESC to reboot.",
                       OptionsMenuList,
                       RTL_NUMBER_OF(OptionsMenuList),
                       SelectedMenuItem, -1,
                       &SelectedMenuItem,
                       TRUE,
                       NULL, NULL))
    {
        /* The user pressed ESC */
        return;
    }

    switch (SelectedMenuItem)
    {
        case 0: // FreeLdr debugging
        {
            CHAR DebugChannelString[100] = "";
            // DebugChannelString[0] = ANSI_NULL;
            if (UiEditBox(FrldrDbgMsg,
                          DebugChannelString,
                          RTL_NUMBER_OF(DebugChannelString)))
            {
                DbgParseDebugChannels(DebugChannelString);
            }
            break;
        }
        // case 1: // Separator
        //     break;
#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
        case 2: // Edit command line
            if (OperatingSystem)
                EditOperatingSystemEntry(OperatingSystem);
            break;
#endif
#ifdef HAS_OPTION_MENU_CUSTOM_BOOT
        case 3: // Custom Boot
            OptionMenuCustomBoot();
            break;
#endif
#ifdef HAS_OPTION_MENU_REBOOT
        case 4: // Reboot
            OptionMenuReboot();
            break;
#endif
    }
    goto doMenu;
}

/*
 * Display selected human-readable boot-option descriptions at the bottom of the screen.
 */
VOID
DisplayBootTimeOptions(
    _In_ OperatingSystemItem* OperatingSystem)
{
    if (!OperatingSystem->AdvBootOptsDesc[0])
        return;

    /* Display the chosen boot options */
    UiDrawText(0,
               UiGetScreenHeight() - 2,
               OperatingSystem->AdvBootOptsDesc,
               ATTR(COLOR_LIGHTBLUE, UiGetMenuBgColor()));
}
