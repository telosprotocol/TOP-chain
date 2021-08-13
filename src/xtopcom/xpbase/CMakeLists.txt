cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xpbase_src)
aux_source_directory(./rlp xpbase_src)
add_library(xpbase ${xpbase_src})

#add_dependencies(xpbase xutility xxbase)
target_link_libraries(xpbase xutility xxbase common)
