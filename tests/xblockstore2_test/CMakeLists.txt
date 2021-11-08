#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

ADD_DEFINITIONS(-DDMOCK_SAVE=1)

aux_source_directory(./ xblockstore_test_dir)
aux_source_directory(tests/mock/xtestdb.cpp xblockstore_test_dir)
add_executable(xblockstore2_test ${xblockstore_test_dir})

add_definitions(-DXENABLE_TESTS)


add_dependencies(xblockstore2_test xblockstore)
include_directories(src/xtopcom/xblockstore/)
target_link_libraries(xblockstore2_test PRIVATE
    xblockstore
    xtxstore
    xblockmaker
    xdata
    gtest
)

if (BUILD_METRICS)
    add_dependencies(xblockstore2_test xmetrics)
    target_link_libraries(xblockstore2_test PRIVATE xmetrics)
endif()
