#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)


aux_source_directory(./ test_dir)
add_executable(xmigrate_test ${test_dir})

target_link_libraries(xmigrate_test PRIVATE
    xmigrate
    xcrypto
    xbase
    gtest
)

if (XENABLE_TESTS)
    enable_testing()
endif()
