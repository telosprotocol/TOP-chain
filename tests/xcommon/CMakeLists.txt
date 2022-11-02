cmake_minimum_required(VERSION 3.8)

add_compile_options(-Wno-unused-parameter)

if (DISABLE_REAL_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

aux_source_directory(./src xcommon_test_src)
add_executable(xcommon_test ${xcommon_test_src})

#add_dependencies(xcommon_test xcommon xxbase)

target_link_libraries(xcommon_test PRIVATE xcommon gtest gtest_main xxbase)
