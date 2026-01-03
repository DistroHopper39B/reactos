
set(NO_VIDEO_GRAPHICS TRUE)
list(APPEND COMPILE_DEFINITIONS TEXT_VGA VidSetScrollRegion=_Unused_VidSetScrollRegion)

list(APPEND SOURCE
    ${CMAKE_CURRENT_LIST_DIR}/pc.h
    ${CMAKE_CURRENT_LIST_DIR}/cmdcnst.h
    ${CMAKE_CURRENT_LIST_DIR}/txtbootvid.c
    ${CMAKE_CURRENT_LIST_DIR}/vga.h)

set(REACTOS_STR_FILE_DESCRIPTION "VGA Text Boot Driver")
#set(REACTOS_STR_ORIGINAL_FILENAME "bootvid.dll")

message("bootvid TARGET: ${_target}")
message("  CONFIG: ${_configfile}")
message("  SOURCE: ${SOURCE}")
message("  compile_definitions: ${COMPILE_DEFINITIONS} ; ${_bviddata_DEFINES}")
