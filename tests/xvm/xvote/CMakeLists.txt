cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xvote_test_src)
add_executable(xvote_test ${xvote_test_src})

#add_dependencies(xvote_test xvm xstore xxbase)

target_link_libraries(xvote_test PRIVATE xvm xstore xxbase gtest luajit)
