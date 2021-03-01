cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xgrpc_test_src)
add_executable(xgrpc_test ${xgrpc_test_src})

target_link_libraries(xgrpc_test PRIVATE xgrpc_mgr xrpc xgrpcservice xtxexecutor jsoncpp gtest pthread)
