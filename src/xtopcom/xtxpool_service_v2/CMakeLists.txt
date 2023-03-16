include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xtxpoolsvr_v2 STATIC ${src_dir})

add_dependencies(xtxpoolsvr_v2 xtxpool_v2 xstore xdata xxbase)

target_link_libraries(xtxpoolsvr_v2 PRIVATE
    xtxpool_v2
    xstore
    xdata
    xxbase
)

if (BUILD_METRICS)
    add_dependencies(xtxpoolsvr_v2 xmetrics)
    target_link_libraries(xtxpoolsvr_v2 PRIVATE xmetrics)
endif()

