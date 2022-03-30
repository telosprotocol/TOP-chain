add_compile_options(-Wpedantic)

aux_source_directory(./src src_dir)
add_library(xevm_common STATIC ${src_dir})