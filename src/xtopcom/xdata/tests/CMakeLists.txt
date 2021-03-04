cmake_minimum_required(VERSION 2.8)

add_compile_options(-Wno-unused-parameter)

aux_source_directory(./ xdata_test_src)
add_executable(xdata_test ${xdata_test_src})

#add_dependencies(xdata_test xdata)

target_link_libraries(xdata_test PRIVATE xdata gtest -ldl -lpthread)
