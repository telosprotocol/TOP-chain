cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xrpc_test_src)

add_executable(xrpc_test ${xrpc_test_src})

#add_dependencies(xrpc_test xrpc)

target_link_libraries(xrpc_test PRIVATE xrpc xelect xdata xtxexecutor xelect_net jsoncpp gtest pthread)

add_subdirectory(xratelimit)
