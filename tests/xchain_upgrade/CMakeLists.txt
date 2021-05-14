cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xchain_upgrade_test_src)

add_executable(xchain_upgrade_test ${xchain_upgrade_test_src})

target_link_libraries(xchain_upgrade_test PRIVATE xchain_upgrade xbasic gtest pthread)