cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src src_dir)
add_library(xplugin STATIC ${src_dir})
target_link_libraries(xplugin PRIVATE xdata)