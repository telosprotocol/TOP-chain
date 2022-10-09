cmake_minimum_required(VERSION 3.8)

add_compile_options(-Wpedantic)

aux_source_directory(./src safebox_src)

add_library(xsafebox STATIC ${safebox_src})

target_link_libraries(xsafebox PRIVATE xhttp xbasic)