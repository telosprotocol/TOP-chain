if (DISABLE_REAL_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()


aux_source_directory(./ xdata_test_dir)
add_executable(xdata_test ${xdata_test_dir})

add_dependencies(xdata_test xblockstore)
include_directories(src/xtopcom/xdata/)
target_link_libraries(xdata_test PRIVATE
    xdata
    gtest
)

if (BUILD_METRICS)
    add_dependencies(xdata_test xmetrics)
    target_link_libraries(xdata_test PRIVATE xmetrics)
endif()
