add_compile_options(-Wno-unused-parameter)

aux_source_directory(./ dir)
add_executable(xdb_test ${dir})

target_link_libraries(xdb_test
    xdb
    -lgtest
)
