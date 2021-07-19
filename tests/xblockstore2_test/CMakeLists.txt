#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)


aux_source_directory(./ xblockstore_test_dir)
aux_source_directory(tests/mock/xtestdb.cpp xblockstore_test_dir)
add_executable(xblockstore2_test ${xblockstore_test_dir})

add_dependencies(xblockstore2_test xblockstore)
include_directories(src/xtopcom/xblockstore/)
target_link_libraries(xblockstore2_test PRIVATE
    xblockstore
    xblockmaker
    xdata
    gtest
)

if (BUILD_METRICS)
    add_dependencies(xblockstore2_test xmetrics)
    target_link_libraries(xblockstore2_test PRIVATE xmetrics)
endif()
