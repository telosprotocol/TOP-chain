find_package(ZLIB REQUIRED)

aux_source_directory(./ xelect_net_demo_dir)
add_executable(xelect_net_demo ${xelect_net_demo_dir})

add_dependencies(xelect_net_demo xelect_net xwrouter xgossip xkad xxbase xpbase)

target_include_directories(xelect_net_demo PUBLIC ${ZLIB_INCLUDE_DIRS})
target_link_libraries(xelect_net_demo xelect_net xwrouter xgossip xkad xtransport xpbase xdata xcrypto xutility xxbase protobuf pthread)
target_link_libraries(xelect_net_demo xrpc xgrpcservice grpc grpc++_unsecure grpc_unsecure gpr cares address_sorting ssl crypto ${ZLIB_LIBRARIES})