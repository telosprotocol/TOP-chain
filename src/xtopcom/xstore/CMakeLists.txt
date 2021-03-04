cmake_minimum_required(VERSION 3.8)

add_compile_options(-static -Wall -Werror)

aux_source_directory(./src src_dir)
add_library(xstore STATIC ${src_dir})

#add_dependencies(xstore xdata xxbase xdb)

target_link_libraries(xstore PRIVATE
    xxbase
    xmbus
    xdata
    xdb
)

if (BUILD_METRICS)
    #add_dependencies(xstore xmetrics)
    target_link_libraries(xstore PRIVATE xmetrics)
endif()
