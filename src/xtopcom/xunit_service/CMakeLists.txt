include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xunit_service STATIC ${src_dir})

add_dependencies(xunit_service xstate_sync xchain_upgrade xconfig xdata xelection xBFT xstore xcommon xxbase)

target_link_libraries(xunit_service PRIVATE
    xstate_sync
    xchain_upgrade
    xconfig
    xdata
    xelection
    xblockmaker
    xtxexecutor
    xstatectx
    xBFT
    xstore
    xcommon
    xxbase
)

if (XENABLE_CODE_COVERAGE)
    target_link_libraries(xunit_service PRIVATE gcov)
endif()

# if (BUILD_METRICS)
#     #add_dependencies(xunit_service xmetrics)
#     target_link_libraries(xunit_service PRIVATE xmetrics)
# endif()
