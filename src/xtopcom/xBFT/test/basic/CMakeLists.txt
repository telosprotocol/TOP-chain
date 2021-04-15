cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ test_src)

add_executable(xBFT_basic_test ${test_src})

add_dependencies(xBFT_basic_test xBFT xxbase)

target_link_libraries(xBFT_basic_test PRIVATE xBFT xxbase)
