cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ tests_src)

add_executable(xgasfee_test ${tests_src})

# add_dependencies(xgas_test xevm xevm_common)
# message("EVM_ENGINE_DIR: " ${EVM_ENGINE_DIR})
target_link_libraries(xgasfee_test PRIVATE xtxexecutor xverifier xvledger xbase xgasfee pthread gtest)

