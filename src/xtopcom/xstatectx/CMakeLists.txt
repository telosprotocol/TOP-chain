include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xstatectx STATIC ${src_dir})

add_dependencies(xstatectx xstatestore xdata xvledger xxbase)
target_link_libraries(xstatectx PRIVATE
    xblockstore
    xstatestore
    xdata
    xvledger
    xxbase
)

if (BUILD_METRICS)
    target_link_libraries(xstatectx PRIVATE xmetrics)
endif()
