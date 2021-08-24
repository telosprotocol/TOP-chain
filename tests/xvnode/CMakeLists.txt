cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xvnode_test_src)

add_executable(xvnode_test ${xvnode_test_src})

add_dependencies(xvnode_test xvnetwork_test_lib xvnode xchain_timer xvnetwork xbasic)
target_link_libraries(xvnode_test PRIVATE xvnetwork_test_lib xvnode xchain_timer xvnetwork xelection xbasic xbase uv dl gtest gtest_main pthread)
