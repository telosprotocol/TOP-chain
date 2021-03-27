if (XENABLE_CODE_COVERAGE)
    add_compile_options(--coverage)
endif()

add_subdirectory(programs/xtopchain)
add_subdirectory(xtopcom)
#add_subdirectory(libraries)
#add_subdirectory(services)
#add_subdirectory(xtools)
