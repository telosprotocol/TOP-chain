cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ tests_src)

add_executable(xgas_test ${tests_src})

# add_dependencies(xgas_test xevm xevm_common)
# message("EVM_ENGINE_DIR: " ${EVM_ENGINE_DIR})
target_link_libraries(xgas_test PRIVATE xvledger xbase xgasfee pthread gtest)

