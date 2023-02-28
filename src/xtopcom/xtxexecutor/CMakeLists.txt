if (NOT DISABLE_EVM)
    add_definitions(-DTXEXECUTOR_ENABLE_EVM)
endif()

if (DISABLE_REAL_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

include_directories(.)
include_directories(./xcontract)
include_directories(./xerror)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xtxexecutor STATIC ${src_dir})

target_link_libraries(xtxexecutor PRIVATE
    xblockstore
    xstore
    xstatectx
    xgasfee
    # xverifier
    xvm
    xchain_upgrade
    xdata
    xxbase
)

if (NOT DISABLE_EVM)
    target_link_libraries(xtxexecutor PRIVATE xevm)
endif()

if (BUILD_METRICS)
    add_dependencies(xtxexecutor xmetrics)
    target_link_libraries(xtxexecutor PRIVATE xmetrics)
endif()
