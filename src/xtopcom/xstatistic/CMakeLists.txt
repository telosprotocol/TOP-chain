include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xstatistic STATIC ${src_dir})

#add_dependencies(xstatistic xxbase)

target_link_libraries(xstatistic PRIVATE
    xxbase
)

if (BUILD_METRICS)
    #add_dependencies(xtxpool_v2 xmetrics)
    target_link_libraries(xstatistic PRIVATE xmetrics)
endif()


