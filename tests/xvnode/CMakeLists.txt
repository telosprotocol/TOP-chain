cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xvnode_test_src)

add_executable(xvnode_test ${xvnode_test_src})

#add_dependencies(xvnode_test xvnode xelect xelect_net xchain_timer xvnetwork xelection xgrpc_mgr xrpc xtxexecutor xelect_net xunit_service xtxpoolsvr xsync xmbus xelect_common xcertauth xbasic)
target_link_libraries(xvnode_test PRIVATE xvnode xelect xelect_net xchain_timer xvnetwork xgrpc_mgr xunit_service xtxpoolsvr xelection xrpc xtxexecutor xelect_net xsync xmbus xelect_common xcertauth xbasic xbase uv dl gtest gtest_main pthread)
