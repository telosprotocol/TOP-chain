aux_source_directory(./src xpbase_src)
# aux_source_directory(./src/xid xpbase_src)
add_library(xpbase ${xpbase_src})

add_dependencies(xpbase xutility xxbase)
target_link_libraries(xpbase xutility xxbase)

if (BUILD_METRICS)
    target_link_libraries(xpbase xmetrics)
endif()