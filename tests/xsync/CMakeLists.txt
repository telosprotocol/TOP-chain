aux_source_directory(./ xsync_test_src)
add_executable(xsync_test ${xsync_test_src})

##add_dependencies(xmbus_test)

target_link_libraries(xsync_test PRIVATE xsync xmbus xvnetwork xdata xstore xblockstore gtest)


aux_source_directory(./mock/ xsync_mock_test_src)
add_executable(xsync_mock_test ${xsync_mock_test_src})
target_link_libraries(xsync_mock_test PRIVATE 
	xsync 
	xmbus 
	xdata 
	xstore 
	xblockstore 
	xvnetwork
	xcommon
	xbase
	xpbase
	gtest
	#tcmalloc_and_profiler
	)
