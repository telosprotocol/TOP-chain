add_compile_options(-Wno-unused-parameter)

aux_source_directory(./ xstore_test_util_dir)
add_executable(xstore_test_util ${xstore_test_util_dir})

#add_dependencies(xstore_test_util xstore xdata)

target_link_libraries(xstore_test_util
    xdata
    xstore
    xutility
    msgpackc
    pthread
)
