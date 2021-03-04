cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ test_src)

add_executable(xtxexecutor_test ${test_src})

#add_dependencies(xtxexecutor_test xtxexecutor xxbase)

target_link_libraries(xtxexecutor_test PRIVATE xverifier xconfig xloader xtxexecutor xstore xxbase gtest pthread)
