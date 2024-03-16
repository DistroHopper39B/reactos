##
## PROJECT:     FreeLoader
## LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
## PURPOSE:     Build definitions for Apple TV
## COPYRIGHT:   Copyright 2024 DistroHopper39B <distrohopper39b.business@gmail.com>
##

# UEFI headers (we have runtime services after all)
include_directories(BEFORE
                    ${REACTOS_SOURCE_DIR}/boot/environ/include/efi
                    ${REACTOS_SOURCE_DIR}/boot/freeldr/freeldr
                    ${REACTOS_SOURCE_DIR}/boot/freeldr/freeldr/include)

spec2def(freeldr_pe.exe freeldr.spec)

# PC stuff
list(APPEND APPLETVLDR_COMMON_ASM_SOURCE
    arch/i386/drvmap.S
    arch/i386/int386.S
    arch/i386/irqsup.S
    arch/i386/pnpbios.S
    arch/i386/entry.S
    # arch/i386/i386trap.S
    arch/i386/linux.S)

# FIXME: Disable Linux booting; it's broken on the TV
list(APPEND APPLETVLDR_BOOTMGR_SOURCE
    ${FREELDR_BOOTMGR_SOURCE}
    linuxboot.c)
    
# Apple TV stuff
list(APPEND APPLETVLDR_BASE_ASM_SOURCE)

list(APPEND APPLETVLDR_BASE_ASM_SOURCE
        arch/i386/appletv/appletventry.S)

list(APPEND APPLETVLDR_ARC_SOURCE
    ${FREELDR_ARC_SOURCE}
    arch/i386/appletv/appletvcons.c
    arch/i386/appletv/appletvdisk.c
    arch/i386/appletv/appletvmem.c
    arch/i386/appletv/appletvrtc.c
    arch/i386/appletv/appletvvideo.c
    arch/i386/appletv/machappletv.c
    arch/i386/appletv/appletvhw.c
    arch/i386/appletv/appletvstubs.c
    arch/i386/pc/pcdisk.c
    arch/i386/pc/pcmem.c)

# extra stuff we need
list(APPEND APPLETVLDR_ARC_SOURCE
    arch/vgafont.c
    arch/drivers/hwide.c
    # disk/scsiport.c
    lib/fs/pxe.c
    # arch/i386/halstub.c
    # arch/i386/ntoskrnl.c
    arch/i386/drivemap.c
    arch/i386/hwapm.c
    arch/i386/hwdisk.c
    arch/i386/hwpci.c
    # arch/i386/i386bug.c
    arch/i386/i386idt.c)

add_asm_files(freeldr_common_asm ${FREELDR_COMMON_ASM_SOURCE} ${APPLETVLDR_COMMON_ASM_SOURCE})

add_library(freeldr_common
    ${freeldr_common_asm}
    ${APPLETVLDR_ARC_SOURCE}
    ${FREELDR_BOOTLIB_SOURCE}
    ${APPLETVLDR_BOOTMGR_SOURCE}
    ${FREELDR_NTLDR_SOURCE})

if(MSVC AND CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # We need to reduce the binary size
    target_compile_options(freeldr_common PRIVATE "/Os")
endif()
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Prevent using SSE (no support in freeldr)
    target_compile_options(freeldr_common PUBLIC -mno-sse)
endif()

set(PCH_SOURCE
    ${APPLETVLDR_ARC_SOURCE}
    ${FREELDR_BOOTLIB_SOURCE}
    ${APPLETVLDR_BOOTMGR_SOURCE}
    ${FREELDR_NTLDR_SOURCE})

add_pch(freeldr_common include/freeldr.h PCH_SOURCE)
add_dependencies(freeldr_common bugcodes asm xdk)

## GCC builds need this extra thing for some reason...
if(ARCH STREQUAL "i386" AND NOT MSVC)
    target_link_libraries(freeldr_common mini_hal)
endif()

add_asm_files(freeldr_base_asm ${APPLETVLDR_BASE_ASM_SOURCE})

list(APPEND APPLETVLDR_BASE_SOURCE
    ${freeldr_base_asm}
    ${FREELDR_BASE_SOURCE}
    arch/i386/appletv/appletvearly.c)

add_executable(freeldr_pe ${APPLETVLDR_BASE_SOURCE})

set_target_properties(freeldr_pe
    PROPERTIES
    ENABLE_EXPORTS TRUE
    DEFINE_SYMBOL "")

if(MSVC)
    target_link_options(freeldr_pe PRIVATE /ignore:4078 /ignore:4254 /DYNAMICBASE:NO /FIXED /FILEALIGN:512 /ALIGN:512)
    add_linker_script(freeldr_pe freeldr_i386.msvc.lds)
    # We don't need hotpatching
    remove_target_compile_option(freeldr_pe "/hotpatch")
    remove_target_compile_option(freeldr_common "/hotpatch")
else()
    target_link_options(freeldr_pe PRIVATE -Wl,--exclude-all-symbols,--file-alignment,0x200,--section-alignment,0x200)
    add_linker_script(freeldr_pe freeldr_gcc.lds)
    # Strip everything, including rossym data
    add_custom_command(TARGET freeldr_pe
                    POST_BUILD
                    COMMAND ${CMAKE_STRIP} --remove-section=.rossym $<TARGET_FILE:freeldr_pe>
                    COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:freeldr_pe>)
endif()

set_image_base(freeldr_pe 0x10000)
set_subsystem(freeldr_pe native)
set_entrypoint(freeldr_pe RealEntryPoint)

target_link_libraries(freeldr_pe mini_hal)

target_link_libraries(freeldr_pe freeldr_common cportlib blcmlib blrtl libcntpr)

# dynamic analysis switches
if(STACK_PROTECTOR)
    target_sources(freeldr_pe PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

if(RUNTIME_CHECKS)
    target_link_libraries(freeldr_pe runtmchk)
    target_link_options(freeldr_pe PRIVATE "/MERGE:.rtc=.text")
endif()

add_dependencies(freeldr_pe asm)

# Apple TV loader stuff
add_custom_target(freeldr
                COMMAND native-pe2macho ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:freeldr_pe> ${CMAKE_CURRENT_BINARY_DIR}/mach_kernel
                DEPENDS native-pe2macho freeldr_pe
                VERBATIM)
                