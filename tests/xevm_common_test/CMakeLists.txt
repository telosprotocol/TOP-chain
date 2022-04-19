cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ tests_src)

add_executable(xevm_common_test ${tests_src})

add_dependencies(xevm_common_test xevm_common xutility xcrypto)
# message("EVM_ENGINE_DIR: " ${EVM_ENGINE_DIR})
target_link_libraries(xevm_common_test PRIVATE xutility xcrypto xevm_common gtest gtest_main pthread dl)

if (BUILD_METRICS)
    target_link_libraries(xevm_common_test PRIVATE xmetrics)
endif()