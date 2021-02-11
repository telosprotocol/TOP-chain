add_compile_options(-Wno-unused-parameter)

aux_source_directory(./ xconfig_test_dir)
add_executable(xconfig_test ${xconfig_test_dir})

target_link_libraries(xconfig_test PRIVATE xconfig xloader xxbase gtest)
