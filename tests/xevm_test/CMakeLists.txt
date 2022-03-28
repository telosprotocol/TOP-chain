cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ tests_src)

add_executable(xtest_evm_test ${tests_src})

add_dependencies(xtest_evm_test xevm xevm_common xutility xcrypto xmetrics)
# message("EVM_ENGINE_DIR: " ${EVM_ENGINE_DIR})
target_link_libraries(xtest_evm_test PRIVATE xutility xcrypto xmetrics xevm xevm_common  gtest gtest_main pthread dl)