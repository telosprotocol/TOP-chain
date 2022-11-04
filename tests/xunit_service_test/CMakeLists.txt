cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xunit_service_test_src)
# aux_source_directory(../mock xunit_service_test_src)
add_executable(xunit_service_test ${xunit_service_test_src})

add_dependencies(xunit_service_test xtxexecutor xunit_service xstate_sync xBFT xvnetwork xxbase xutility xcommon)

target_link_libraries(xunit_service_test PRIVATE xtxexecutor xunit_service xstate_sync xBFT xvnetwork xxbase xutility xcommon gtest gmock)



# aux_source_directory(./mock/ xtc_mock_test_src)
# add_executable(xtc_mock_test ${xtc_mock_test_src})
# target_link_libraries(xtc_mock_test PRIVATE xunit_service xBFT xvnetwork xxbase xutility xcommon)
