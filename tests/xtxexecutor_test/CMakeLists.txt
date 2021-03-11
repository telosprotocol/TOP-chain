#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)


aux_source_directory(./ xtxexecutor2_test_dir)
add_executable(xtxexecutor2_test ${xtxexecutor2_test_dir})

add_dependencies(xtxexecutor2_test xtxexecutor)
# include_directories(src/xtopcom/xblockstore/)
target_link_libraries(xtxexecutor2_test PRIVATE
    xblockstore
    xtxexecutor
    gtest
)

# if (BUILD_METRICS)
#     add_dependencies(xtxexecutor2_test xmetrics)
#     target_link_libraries(xtxexecutor2_test PRIVATE xmetrics)
# endif()

if (XENABLE_TESTS)
    enable_testing()
endif()
