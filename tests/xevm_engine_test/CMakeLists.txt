cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ tests_src)
aux_source_directory(./evm_test_fixture evm_test_fixture)

add_executable(xevm_engine_test ${tests_src} ${evm_test_fixture})

add_dependencies(xevm_engine_test xevm xevm_contract_runtime xevm_engine xcommon)
get_target_property(EVM_ENGINE_DIR xevm_engine LOCATION)
# message("EVM_ENGINE_DIR: " ${EVM_ENGINE_DIR})
target_link_libraries(xevm_engine_test PRIVATE xevm xevm_contract_runtime xcommon ${EVM_ENGINE_DIR}/libxevm_engine.a xevm_runner gtest pthread dl)