cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xchain_timer_test_src)
add_executable(xchain_timer_test ${xchain_timer_test_src})

#add_dependencies(xchain_timer_test xchain_timer xxbase)

target_link_libraries(xchain_timer_test PRIVATE xchain_timer xxbase gtest gmock)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(xchain_timer_test PRIVATE xbase)
endif()
