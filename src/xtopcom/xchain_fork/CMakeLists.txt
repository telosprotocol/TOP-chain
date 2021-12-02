cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src src_dir)

add_library(xchain_fork STATIC ${src_dir})

# target_link_libraries(xchain_fork PRIVATE)
