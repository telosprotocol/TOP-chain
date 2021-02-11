cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xcodec_src)
add_library(xcodec STATIC ${xcodec_src})

target_link_libraries(xcodec PRIVATE msgpackc)
