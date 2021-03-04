cmake_minimum_required(VERSION 3.8)

add_compile_options(-static)
add_compile_options(-Wall)
add_compile_options(-Werror)

aux_source_directory(./src src_dir)
add_library(xgrpcservice STATIC ${src_dir})

#add_dependencies(xgrpcservice xxbase)

target_link_libraries(xgrpcservice PRIVATE
    xvnetwork
    xcommon
    xxbase
    msgpackc
    xcodec
    pthread
)
