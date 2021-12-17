cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xloader_test_src)

add_executable(xloader_test ${xloader_test_src})

target_link_libraries(xloader_test PRIVATE xloader xstore xxbase gtest pthread)
