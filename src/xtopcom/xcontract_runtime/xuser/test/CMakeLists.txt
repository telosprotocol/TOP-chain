cmake_minimum_required(VERSION 3.8)
aux_source_directory(./ xwasm_contract_src)





# target_link_libraries(xwasm_contract_test PRIVATE xcontract_runtime xvm lua xloader xxbase pthread jsoncpp gtest pthread xblockstore ${CMAKE_CURRENT_SOURCE_DIR}/librustvm.a dl)

add_executable(xwasm_contract_test ${xwasm_contract_src})

add_dependencies(xwasm_contract_test xcontract_runtime xvm xloader xxbase xblockstore xwasm_vm)
target_link_libraries(xwasm_contract_test PRIVATE xcontract_runtime xvm lua xloader xxbase pthread jsoncpp gtest pthread xblockstore xwasm_vm dl)
