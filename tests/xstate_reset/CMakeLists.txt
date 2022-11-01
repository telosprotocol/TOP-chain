cmake_minimum_required(VERSION 3.8)

# add_compile_options(-Wpedantic)

aux_source_directory(./src state_reset_test_src)

add_executable(xstate_reset_test ${state_reset_test_src})

target_link_libraries(xstate_reset_test PRIVATE xstate_reset xbasic gtest pthread)