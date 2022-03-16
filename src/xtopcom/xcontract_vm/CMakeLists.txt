add_compile_options(-Wpedantic)

aux_source_directory(./src src_dir)
add_library(xcontract_vm STATIC ${src_dir})

add_dependencies(xcontract_vm xcommon xbasic xcontract_common xsystem_contract_runtime)

target_link_libraries(xcontract_vm PRIVATE xcommon xbasic xcontract_common xsystem_contract_runtime)
