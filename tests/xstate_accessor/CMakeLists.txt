cmake_minimum_required(VERSION 3.8)

# add_compile_options(-Wpedantic)

aux_source_directory(./src state_accessor_test_src)

add_executable(xstate_accessor_test ${state_accessor_test_src})

add_dependencies(xstate_accessor_test xstate_accessor xdb xstore xgenesis xtxstore xevm_contract_runtime)
target_link_libraries(xstate_accessor_test PRIVATE xstate_accessor xdb xstore xtxstore xgenesis xevm_contract_runtime xevm_common xvledger xcommon xbasic gtest pthread)
