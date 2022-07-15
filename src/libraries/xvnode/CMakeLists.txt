aux_source_directory(./src xvnode_src)

add_library(xvnode STATIC ${xvnode_src})

add_dependencies(xvnode xgrpc_mgr xunit_service xtxpoolsvr_v2 xsync)
target_link_libraries(xvnode PRIVATE xgrpc_mgr xunit_service xtxpoolsvr_v2 xsync)
