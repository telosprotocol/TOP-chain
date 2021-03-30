cmake_minimum_required(VERSION 3.8)

add_compile_options(-static)

aux_source_directory(./src xthreadpool_src)

add_library(xthreadpool STATIC ${xthreadpool_src})

add_compile_options(-fpermissive)

