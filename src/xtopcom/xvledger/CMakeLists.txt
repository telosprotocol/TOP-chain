#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)

aux_source_directory(./src src_dir)
add_library(xvledger STATIC ${src_dir})

target_link_libraries(xvledger PRIVATE
    xbase
)

# if (XENABLE_TESTS)
#     enable_testing()
#     add_subdirectory(test)
# endif()
