cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ tests_src)
aux_source_directory(./evm_test_fixture evm_test_fixture)

add_executable(xtest_evm_engine_test ${tests_src} ${evm_test_fixture})

add_dependencies(xtest_evm_engine_test xevm_engine)
get_target_property(EVM_ENGINE_DIR xevm_engine LOCATION)
# message("EVM_ENGINE_DIR: " ${EVM_ENGINE_DIR})
target_link_libraries(xtest_evm_engine_test PRIVATE ${EVM_ENGINE_DIR}/libxevm_engine.a xevm_runner gtest gtest_main pthread dl)