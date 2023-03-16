aux_source_directory(./src src_dir)
add_library(xstore STATIC ${src_dir})

add_dependencies(xstore xchain_fork xmbus xstatectx xdb xdata xxbase)

target_link_libraries(xstore PRIVATE
    xchain_fork
    xmbus
    xdata
    xstatectx
    xdb
    xxbase
)

if (BUILD_METRICS)
    #add_dependencies(xstore xmetrics)
    target_link_libraries(xstore PRIVATE xmetrics)
endif()
