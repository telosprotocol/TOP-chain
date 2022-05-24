cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ src)

add_executable(xevm_contract_runtime_test ${src})

# add_dependencies(xevm_contract_runtime_test xevm_common xutility xcrypto)
target_link_libraries(xevm_contract_runtime_test PRIVATE xevm_contract_runtime xevm_common gtest pthread)