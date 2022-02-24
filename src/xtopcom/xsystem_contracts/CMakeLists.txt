add_compile_options(-Wall -Werror -Wpedantic)

aux_source_directory(./src src_dir)
add_library(xsystem_contract STATIC ${src_dir})

add_dependencies(xsystem_contract xcommon xbasic xcontract_common xcontract_runtime)

target_link_libraries(xsystem_contract PRIVATE xcommon xbasic xcontract_common xcontract_runtime)
