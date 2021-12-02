#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)


aux_source_directory(./ test_dir)
add_executable(xvledger_test ${test_dir})

target_link_libraries(xvledger_test PRIVATE
    xvledger
    xbase
    xutility
    gtest
)

if (XENABLE_TESTS)
    enable_testing()
endif()

if (BUILD_METRICS)
    target_link_libraries(xvledger_test PRIVATE xmetrics)
endif()
