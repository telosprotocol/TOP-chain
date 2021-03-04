cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xrouter_test_src)
add_executable(xrouter_test ${xrouter_test_src})

#add_dependencies(xrouter_test xconfig xrouter xcommon)

target_link_libraries(xrouter_test PRIVATE xconfig xrouter xcommon gtest)