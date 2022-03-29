cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xelection_test_src)

add_library(xelection_lib ${xelection_test_src})
add_executable(xelection_test ${xelection_test_src})

#add_dependencies(xelection_test xelection)
target_link_libraries(xelection_test PRIVATE xelection xchain_fork gtest gtest_main xunit_service)
