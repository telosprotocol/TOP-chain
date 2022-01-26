aux_source_directory(./archive/ xsync_test_src)
add_executable(xsync_test ${xsync_test_src})

#add_dependencies(xmbus_test)

target_link_libraries(xsync_test PRIVATE xsync xmbus xvnetwork xdata xstore xblockstore gtest)
