cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xverifier_test_src)

add_executable(xverifier_test ${xverifier_test_src})

add_dependencies(xverifier_test xverifier xxbase xstore xblock xloader)

target_link_libraries(xverifier_test PRIVATE xverifier xloader xstore xblock xxbase gtest pthread jsoncpp)
