include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xstatestore STATIC ${src_dir})

add_dependencies(xstatestore xblockstore xmbus xstate_mpt xdata xcommon xvledger xxbase)
target_link_libraries(xstatestore PRIVATE
    xblockstore
    xmbus
    xstate_mpt
    xdata
    xcommon
    xvledger
    xxbase
)

if (BUILD_METRICS)
    target_link_libraries(xstatestore PRIVATE xmetrics)
endif()
