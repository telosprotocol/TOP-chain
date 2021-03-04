add_compile_options(-Wno-unused-parameter)

aux_source_directory(./ xstore_test_dir)
add_executable(xstore_test ${xstore_test_dir})

#add_dependencies(xstore_test xstore xdata)

target_link_libraries(xstore_test
    xdata
    xstore
    xutility
    msgpackc
    pthread
    gtest
)
