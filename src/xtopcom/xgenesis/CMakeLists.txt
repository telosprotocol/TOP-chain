cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src src_dir)
add_library(xgenesis STATIC ${src_dir})

#add_dependencies(xgenesis xxbase)

target_link_libraries(xgenesis PRIVATE xxbase)
