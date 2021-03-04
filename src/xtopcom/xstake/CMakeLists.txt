cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src src_dir)
add_library(xstake STATIC ${src_dir})

#add_dependencies(xstake xdata xxbase)

target_link_libraries(xstake PRIVATE
    xxbase
    xdata
    xstore
)

if (BUILD_METRICS)
    #add_dependencies(xstake xmetrics)
    target_link_libraries(xstake PRIVATE xmetrics)
endif()
