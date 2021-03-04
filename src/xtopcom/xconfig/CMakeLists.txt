cmake_minimum_required(VERSION 3.8)

add_compile_options(-static)

aux_source_directory(./src src_dir)

add_library(xconfig STATIC ${src_dir})

#add_dependencies(xconfig xbasic xpbase)

target_link_libraries(xconfig PRIVATE xbasic xpbase)

