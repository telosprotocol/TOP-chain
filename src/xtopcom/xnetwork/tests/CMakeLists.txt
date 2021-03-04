cmake_minimum_required(VERSION 3.8)

add_compile_options(-Wno-unused-parameter)

aux_source_directory(./src xnetwork_test_src)
add_executable(xnetwork_test ${xnetwork_test_src})

#add_dependencies(xnetwork_test xnetwork xcommon xxbase)

target_link_libraries(xnetwork_test PRIVATE xnetwork xcommon gtest gtest_main xxbase)
