#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)


aux_source_directory(./ xblockstore_test_dir)
add_executable(xblockstore_test ${xblockstore_test_dir})

#add_dependencies(xblockstore_test xblockstore)
include_directories(src/xtopcom/xblockstore/)
target_link_libraries(xblockstore_test PRIVATE
    xblockstore
    gtest
)

# if (BUILD_METRICS)
#     #add_dependencies(xblockstore_test xmetrics)
#     target_link_libraries(xblockstore_test PRIVATE xmetrics)
# endif()

if (XENABLE_TESTS)
    enable_testing()
endif()
