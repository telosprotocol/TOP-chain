cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xnetwork_src)

add_library(xnetwork STATIC ${xnetwork_src})

#add_dependencies(xnetwork xcodec xbasic xutility xxbase)
target_link_libraries(xnetwork PRIVATE xcodec xbasic xutility xxbase pthread)
