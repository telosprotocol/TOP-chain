aux_source_directory(./src xutility_test_src)
add_executable(xutility_test ${xutility_test_src})

#add_dependencies(xutility_test xutility xbasic xxbase)

target_link_libraries(xutility_test PRIVATE xutility xbasic xxbase gtest)
