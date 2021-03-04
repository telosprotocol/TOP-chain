cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xBFT_test_src)
add_executable(xBFT_test ${xBFT_test_src})

#add_dependencies(xBFT_test xBFT xbasic xxbase)

target_link_libraries(xBFT_test PRIVATE xBFT xbasic xxbase gtest gmock)
