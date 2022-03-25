add_compile_options(-Wpedantic)

aux_source_directory(./src src_dir)
add_library(xevm_contract_runtime STATIC ${src_dir})

add_dependencies(xevm_contract_runtime xcommon xbasic xcontract_common xcontract_runtime)

target_link_libraries(xevm_contract_runtime PRIVATE xcommon xbasic xcontract_common xcontract_runtime)
