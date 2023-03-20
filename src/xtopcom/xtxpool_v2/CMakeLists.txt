include_directories(.)
include_directories(./src)

if (DISABLE_REAL_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

aux_source_directory(./src src_dir)
add_library(xtxpool_v2 STATIC ${src_dir})

add_dependencies(xtxpool_v2 xdata xblockstore xxbase)

target_link_libraries(xtxpool_v2 PRIVATE
    xdata
    xblockstore
    xstate_mpt
    xxbase
)

if (BUILD_METRICS)
    add_dependencies(xtxpool_v2 xmetrics)
    target_link_libraries(xtxpool_v2 PRIVATE xmetrics)
endif()


