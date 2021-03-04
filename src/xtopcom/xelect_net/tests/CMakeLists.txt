
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    aux_source_directory(./ xelect_net_test_dir)
    add_executable(xelect_net_test ${xelect_net_test_dir})
    #add_dependencies(xelect_net_test xelect_net )
    target_link_libraries(xelect_net_test xelect_net xvnetwork xdata gtest gmock)

    if(XENABLE_CODE_COVERAGE)
        target_link_libraries(xelect_net_test gcov)
    endif()
endif()
