cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xzlib_test_src)
add_executable(xzlib_test ${xzlib_test_src})

#add_dependencies(xzlib_test xzlib)

target_link_libraries(xzlib_test PRIVATE xzlib gtest pthread)
