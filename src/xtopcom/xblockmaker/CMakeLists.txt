include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xblockmaker STATIC ${src_dir})

target_link_libraries(xblockmaker PRIVATE
    xtxpool_v2
    xtxexecutor
    xblockstore
    xstore
    xstatectx
    xstate_reset
    #xverifier
    xvm
    xchain_upgrade
    xdata
    xxbase
    intel_aes64
    xcontract_vm
    xstate_mpt
    xgenesis
)

if (BUILD_METRICS)
    target_link_libraries(xblockmaker PRIVATE xmetrics)
endif()

