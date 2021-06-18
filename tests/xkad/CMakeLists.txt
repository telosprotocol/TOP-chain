aux_source_directory(./ xkad_test_src)
add_executable(xkad_test ${xkad_test_src})

add_dependencies(xkad_test xpbase xcommon)
target_link_libraries(xkad_test PRIVATE xpbase xxbase xcommon gtest gtest_main pthread)
