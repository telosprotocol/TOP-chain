cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xstatistic_test_src)

add_executable(xstatistic_test ${xstatistic_test_src})

target_link_libraries(xstatistic_test PRIVATE xstatistic xxbase secp256k1 xconfig xloader xgenesis gtest pthread)
