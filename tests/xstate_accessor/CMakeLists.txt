cmake_minimum_required(VERSION 3.8)

# add_compile_options(-Wpedantic)

aux_source_directory(./src state_accessor_test_src)

add_executable(xstate_accessor_test ${state_accessor_test_src})

target_link_libraries(xstate_accessor_test PRIVATE xevm_common xvledger xcommon xbasic gtest gtest_main pthread)
