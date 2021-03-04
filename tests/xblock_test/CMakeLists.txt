cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xblock_test_src)
aux_source_directory(../mock xblock_test_src)
add_executable(xblock_test ${xblock_test_src})

#add_dependencies(xblock_test xtxexecutor xunit_service xBFT xvnetwork xxbase xutility xcommon)

target_link_libraries(xblock_test PRIVATE xtxexecutor xunit_service xBFT xvnetwork xxbase xutility xcommon gtest gmock)
