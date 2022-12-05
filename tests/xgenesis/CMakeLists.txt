cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src genesis_test_src)
aux_source_directory(./ main)

add_executable(xgenesis_test ${genesis_test_src} ${main})

#add_dependencies(xgenesis_test xxbase xstore)

target_link_libraries(xgenesis_test PRIVATE xloader xgenesis xtxstore xblockstore xstatestore xstore xxbase xcommon gtest pthread jsoncpp)
