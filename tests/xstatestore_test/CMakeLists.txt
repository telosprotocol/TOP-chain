aux_source_directory(./ _dir)
aux_source_directory(tests/mock/xtestdb.cpp _dir)
add_executable(xstatestore_test ${_dir})

add_dependencies(xstatestore_test xstatestore)
include_directories(src/xtopcom/xstatestore/)
target_link_libraries(xstatestore_test PRIVATE
    xdbstore
    xblockstore
    xstatestore
    xtxstore
    xblockmaker
    xdata
    xgenesis
    xloader
    gtest
)

if (BUILD_METRICS)
    add_dependencies(xstatestore_test xmetrics)
    target_link_libraries(xstatestore_test PRIVATE xmetrics)
endif()
