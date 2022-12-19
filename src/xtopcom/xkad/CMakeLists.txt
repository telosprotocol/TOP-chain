cmake_minimum_required(VERSION 3.8)

find_package(Protobuf REQUIRED)

include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/proto)

aux_source_directory(./src xkad_src)
aux_source_directory(./proto xkad_src)
add_library(xkad ${xkad_src})

include_directories(${Protobuf_INCLUDE_DIRS})
# include_directories(${CMAKE_CURRENT_BINARY_DIR})
#add_dependencies(xkad xpbase xtransport xdb)
target_link_libraries(xkad xpbase xtransport xdb ${Protobuf_LIBRARIES})

