cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xdata_test_src)
add_executable(xdata2_test ${xdata_test_src})

add_dependencies(xdata2_test xdata xbasic)

target_link_libraries(xdata2_test PRIVATE xdata xbasic gtest gtest_main)
