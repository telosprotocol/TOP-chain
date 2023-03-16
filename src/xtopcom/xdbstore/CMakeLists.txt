aux_source_directory(./src src_dir)
add_library(xdbstore STATIC ${src_dir})

add_dependencies(xdbstore xdata xxbase xdb)

target_link_libraries(xdbstore PRIVATE
    xxbase
    xvledger
    xdb
)

if (BUILD_METRICS)
    #add_dependencies(xdbstore xmetrics)
    target_link_libraries(xdbstore PRIVATE xmetrics)
endif()
