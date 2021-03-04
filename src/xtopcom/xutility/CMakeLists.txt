aux_source_directory(./src xutility_src)
add_library(xutility STATIC ${xutility_src})

# #add_dependencies(xutility )

target_link_libraries(xutility PRIVATE trezor-crypto)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (XENABLE_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()
