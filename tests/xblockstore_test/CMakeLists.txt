aux_source_directory(./ xblockstore_test_dir)
aux_source_directory(tests/mock/xtestdb.cpp xblockstore_test_dir)
add_executable(xblockstore_test ${xblockstore_test_dir})

add_dependencies(xblockstore_test xblockstore)
include_directories(src/xtopcom/xblockstore/)
target_link_libraries(xblockstore_test PRIVATE
    xdbstore
    xblockstore
    xtxstore
    xblockmaker
    xdata
    xgenesis
    xloader
    gtest
)

if (BUILD_METRICS)
    add_dependencies(xblockstore_test xmetrics)
    target_link_libraries(xblockstore_test PRIVATE xmetrics)
endif()
