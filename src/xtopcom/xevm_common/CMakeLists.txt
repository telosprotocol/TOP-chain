add_compile_options(-Wpedantic)

aux_source_directory(./src src_dir)
add_library(xevm_common STATIC ${src_dir})

add_dependencies(xevm_common xcommon xbasic)

target_link_libraries(xevm_common PRIVATE xcommon xbasic)