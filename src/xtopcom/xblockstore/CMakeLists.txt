include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xblockstore STATIC ${src_dir})

add_dependencies(xblockstore xdbstore xdata xxbase)

target_link_libraries(xblockstore PRIVATE
    xdbstore
    xdata
    xxbase
)

if (BUILD_METRICS)
    add_dependencies(xblockstore xmetrics)
    target_link_libraries(xblockstore PRIVATE xmetrics)
endif()

