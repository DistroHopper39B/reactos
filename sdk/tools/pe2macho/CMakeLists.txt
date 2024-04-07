list(APPEND SOURCE
    pe2macho.c)

add_host_tool(pe2macho ${SOURCE})

if(NOT MSVC)
    target_compile_options(pe2macho PRIVATE "-fshort-wchar")
endif()

target_link_libraries(pe2macho PRIVATE host_includes)