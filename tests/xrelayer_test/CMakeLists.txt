cmake_minimum_required(VERSION 3.8)

add_compile_options(-static)

aux_source_directory(./ test_dir)

add_executable(xrelayer_test ${test_dir})

add_dependencies(xrelayer_test xrelayer xutility xcrypto)

target_link_libraries(xrelayer_test PRIVATE xutility xcrypto xcrypto xrelayer  gtest gtest_main pthread dl)
