list(APPEND SOURCE
        genmachoheader.c)

add_host_tool(genmachoheader ${SOURCE})

if(NOT MSVC)
    target_compile_options(genmachoheader PRIVATE "-fshort-wchar")
endif()

target_link_libraries(genmachoheader PRIVATE host_includes)
