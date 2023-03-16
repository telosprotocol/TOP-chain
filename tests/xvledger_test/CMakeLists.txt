aux_source_directory(./ test_dir)
add_executable(xvledger_test ${test_dir})

target_link_libraries(xvledger_test PRIVATE
    xvledger
    xbase
    xutility
    gtest
)

if (BUILD_METRICS)
    target_link_libraries(xvledger_test PRIVATE xmetrics)
endif()
