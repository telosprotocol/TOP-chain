cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ test_src)
add_executable(xchain_config_test ${test_src})

target_link_libraries(xchain_config_test PRIVATE xchain_config xxbase gtest)