cmake_minimum_required(VERSION 3.8)

add_compile_options(-static)

aux_source_directory(./src src_dir)

add_library(xrelay_chain STATIC ${src_dir})

target_link_libraries(xrelay_chain PRIVATE xvm)

target_link_libraries(xrelay_chain PRIVATE
    xdata
    xtxexecutor
    xstore
    xcommon
    xxbase
)