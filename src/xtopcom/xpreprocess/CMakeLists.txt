# cmake_minimum_required(VERSION 3.8)
# add_compile_options(-static)

aux_source_directory(./src src_dir)
add_library(xpreprocess STATIC ${src_dir})
target_link_libraries(xpreprocess PRIVATE xdata xplugin)