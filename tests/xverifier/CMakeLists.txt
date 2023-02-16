cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xverifier_test_src)

add_executable(xverifier_test ${xverifier_test_src})

#add_dependencies(xverifier_test xverifier xxbase xstore xloader)

target_link_libraries(xverifier_test PRIVATE xdata xdbstore xloader xxbase gtest pthread jsoncpp)
