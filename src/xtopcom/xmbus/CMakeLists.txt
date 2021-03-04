cmake_minimum_required(VERSION 3.8)

add_compile_options(-static -Wall -Werror)

aux_source_directory(./src src_dir)

add_library(xmbus STATIC ${src_dir})

#add_dependencies(xmbus xvnetwork xbasic)
target_link_libraries(xmbus PRIVATE xvnetwork xbasic)

if (BUILD_METRICS)
    #add_dependencies(xmbus xmetrics)
    target_link_libraries(xmbus PRIVATE xmetrics)
endif()
