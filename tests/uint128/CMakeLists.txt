cmake_minimum_required(VERSION 3.8)

include_directories(../../src/xtopcom/xbasic)
aux_source_directory(./ main_src)
aux_source_directory(./testcases/ testcases)

add_executable(uint128_test ${main_src} ${testcases})

#add_dependencies(uint128_test xbasic)
target_link_libraries(uint128_test PRIVATE xbasic gtest)
