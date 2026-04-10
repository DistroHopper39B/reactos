/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FreeLoader Setup and Configuration F2 menu.
 * COPYRIGHT:   Copyright 2022-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

VOID
FreeLdrSetupMenu(
    _In_opt_ OperatingSystemItem* OperatingSystem);

VOID
DisplayBootTimeOptions(
    _In_ OperatingSystemItem* OperatingSystem);
