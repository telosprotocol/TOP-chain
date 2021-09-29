aux_source_directory(./src xvnode_src)
aux_source_directory(./xcomponents/xvnode_sniff xvnode_src)
aux_source_directory(./xcomponents/xvnode_util xvnode_src)

add_library(xvnode STATIC ${xvnode_src})

add_dependencies(xvnode xgrpc_mgr xunit_service xtxpoolsvr_v2 xsync)
target_link_libraries(xvnode PRIVATE xgrpc_mgr xunit_service xtxpoolsvr_v2 xsync)
