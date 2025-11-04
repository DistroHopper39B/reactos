/*
 * PROJECT:     Freeldr UEFI Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     UEFI Apple Screen Info Protocol
 * COPYRIGHT:   Copyright 2025 Sylas Hollander <distrohopper39b.business@gmail.com>
 */

#pragma once

/* Apple's proprietary screen info protocol */
#define APPLE_SCREEN_INFO_PROTOCOL_GUID {0xe316e100, 0x0751, 0x4c49, {0x90, 0x56, 0x48, 0x6c, 0x7e, 0x47, 0x29, 0x03}}

typedef struct _APPLE_SCREEN_INFO_PROTOCOL APPLE_SCREEN_INFO_PROTOCOL;

typedef EFI_STATUS (EFIAPI *GetAppleScreenInfo)(APPLE_SCREEN_INFO_PROTOCOL *This,
													UINT64 *BaseAddress,
													UINT64 *FrameBufferSize,
													UINT32 *BytesPerRow,
													UINT32 *Width,
													UINT32 *Height,
													UINT32 *Depth);
													
struct _APPLE_SCREEN_INFO_PROTOCOL {
	GetAppleScreenInfo	GetInfo;
};
