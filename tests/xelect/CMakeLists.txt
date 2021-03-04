cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xrelect_test_src)
add_executable(xelect_test ${xrelect_test_src})

#add_dependencies(xelect_test xelect xvnetwork xxbase)

target_link_libraries(xelect_test PRIVATE xelect xvnetwork xvrf xstore xxbase xbasic pthread jsoncpp gtest pthread)
