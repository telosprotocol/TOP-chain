cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src src_dir)

add_library(xchain_timer STATIC ${src_dir})

#add_dependencies(xchain_timer xdata xxbase)

target_link_libraries(xchain_timer PRIVATE xxbase xdata pthread)

