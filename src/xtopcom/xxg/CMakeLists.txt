cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src SRCS)
add_library(xxg ${SRCS})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(xxg xg uuid)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(xxg xg ${CFLIB})
endif()
