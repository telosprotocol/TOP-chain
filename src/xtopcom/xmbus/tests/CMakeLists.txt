aux_source_directory(./src xmbus_test_src)
add_executable(xmbus_test ${xmbus_test_src})

#add_dependencies(xmbus_test)

target_link_libraries(xmbus_test PRIVATE xmbus gtest)
