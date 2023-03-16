aux_source_directory(../xsystem_contract/xelection_algorithm/fixture xelection_algorithm_fixture)
aux_source_directory(./ xelect_net_test_dir)
add_executable(xelect_net_test ${xelection_algorithm_fixture} ${xelect_net_test_dir})

add_dependencies(xelect_net_test xelect_net xvm xcommon)
target_link_libraries(xelect_net_test PRIVATE xelect_net xkad xwrouter xvm xcommon gtest pthread)

if (BUILD_METRICS)
    add_dependencies(xelect_net_test xmetrics)
    target_link_libraries(xelect_net_test PRIVATE xmetrics)
endif()
