cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xcommon_src)
add_library(xcommon STATIC ${xcommon_src})

add_dependencies(xcommon xutility xcodec xbasic)
target_link_libraries(xcommon PRIVATE xutility xcodec xbasic)
