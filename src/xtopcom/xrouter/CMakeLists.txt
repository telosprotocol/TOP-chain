cmake_minimum_required(VERSION 3.8)

add_compile_options(-static)

aux_source_directory(./src src_dir)

add_library(xrouter STATIC ${src_dir})

# #add_dependencies(xrouter xvm)

target_link_libraries(xrouter PRIVATE xvm)

