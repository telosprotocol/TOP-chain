#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)


aux_source_directory(./ test_dir)
add_executable(xindexstore_test ${test_dir})

target_link_libraries(xindexstore_test PRIVATE
    xindexstore
    gtest
)

if (XENABLE_TESTS)
    enable_testing()
endif()
