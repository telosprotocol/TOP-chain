
if (XENABLE_CODE_COVERAGE)
    add_compile_options(--coverage)
    link_libraries(gcov)
endif()

add_subdirectory(xtopcom)
add_subdirectory(libraries)
add_subdirectory(programs)
add_subdirectory(services)
add_subdirectory(xtools)
