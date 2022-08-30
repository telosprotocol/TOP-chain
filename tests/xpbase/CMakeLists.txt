cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ test_src)

add_executable(xpbase_test ${test_src})

target_link_libraries(xpbase_test PRIVATE xxbase gtest pthread jsoncpp)
