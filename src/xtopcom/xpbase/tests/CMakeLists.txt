
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    aux_source_directory(./ xpbase_test_dir)
    add_executable(xpbase_test ${xpbase_test_dir})
    #add_dependencies(xpbase_test xpbase)
    target_link_libraries(xpbase_test xpbase gtest gmock trezor-crypto)

    if(XENABLE_CODE_COVERAGE)
        target_link_libraries(xpbase_test gcov)
    endif()
endif()
