cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ main)
aux_source_directory(./src src)
add_executable(xcheckpoint_test ${main} ${src})

add_dependencies(xcheckpoint_test xtxstore xblockstore xblockmaker xgenesis xdata)

target_link_libraries(xcheckpoint_test PRIVATE xtxstore xblockstore xblockmaker xgenesis xdata gtest)