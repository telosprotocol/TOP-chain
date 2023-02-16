aux_source_directory(./ xtxexecutor_test_dir)
add_executable(xtxexecutor_test ${xtxexecutor_test_dir})

add_dependencies(xtxexecutor_test xtxexecutor)
include_directories(src/xtopcom/xtxexecutor/)
target_link_libraries(xtxexecutor_test PRIVATE xdata xconfig xloader xtxexecutor xstore xxbase gtest pthread)

if (BUILD_METRICS)
    add_dependencies(xtxexecutor_test xmetrics)
    target_link_libraries(xtxexecutor_test PRIVATE xmetrics)
endif()
