cmake_minimum_required(VERSION 3.8)

add_compile_options(-Wno-unused-parameter)

aux_source_directory(./src xstake_algorithm_test_src)
add_executable(xstake_algorithm_test ${xstake_algorithm_test_src})

#add_dependencies(xstake_algorithm_test xdata xvm xstore xxbase)

target_link_libraries(xstake_algorithm_test PRIVATE xdata gtest xvm xstore lua xxbase jsoncpp -ldl -lpthread)