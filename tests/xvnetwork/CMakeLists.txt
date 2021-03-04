cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xvnetwork_test_src)

add_executable(xvnetwork_test ${xvnetwork_test_src})

#add_dependencies(xvnetwork_test xvnetwork)

target_link_libraries(xvnetwork_test PRIVATE xcommon xutility xvnetwork xnetwork xchain_timer gtest gtest_main pthread )