cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ tests_src)
aux_source_directory(./evm_test_fixture evm_test_fixture)

add_executable(test_aurora ${tests_src} ${evm_test_fixture})

add_dependencies(test_aurora xevm_engine)
get_target_property(EVM_ENGINE_DIR xevm_engine LOCATION)
# message("EVM_ENGINE_DIR: " ${EVM_ENGINE_DIR})
target_link_libraries(test_aurora PRIVATE ${EVM_ENGINE_DIR}/libxevm_engine.a xevm_runtime gtest gtest_main pthread dl)