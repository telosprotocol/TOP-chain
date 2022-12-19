cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xbasic_test_src)

add_executable(xbasic_test ${xbasic_test_src})

add_dependencies(xbasic_test xbasic)
target_link_libraries(xbasic_test PRIVATE xbasic xbase gtest gtest_main)
