add_compile_options(-Wpedantic)

aux_source_directory(./src src_dir)
add_library(xsystem_contract_runtime STATIC ${src_dir})

add_dependencies(xsystem_contract_runtime xcommon xbasic xcontract_common xcontract_runtime xsystem_contract)

target_link_libraries(xsystem_contract_runtime PRIVATE xcommon xbasic xcontract_common xcontract_runtime xsystem_contract xvm)
