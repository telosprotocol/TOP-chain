add_compile_options(-Wpedantic)

aux_source_directory(./src src_dir)
add_library(xevm STATIC ${src_dir})

add_dependencies(xevm xcommon xbasic xcontract_common xevm_contract_runtime)

target_link_libraries(xevm PRIVATE xcommon xbasic xcontract_common xevm_contract_runtime)
