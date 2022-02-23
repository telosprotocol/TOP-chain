cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xrpc_test_src)
# aux_source_directory(./ws xrpc_ws)
#add_executable(xrpc_test ${xrpc_test_src} ${xrpc_ws})
#add_executable(xrpc_test  ${xrpc_ws})
add_executable(xrpc_test ${xrpc_test_src})

# add_dependencies(xrpc_test xrpc xchaininit xxbase)

target_link_libraries(xrpc_test PRIVATE xrpc xvnetwork xelect_net xtxstore xloader xgenesis xxbase 
                      xdb
                      rocksdb zstd snappy bz2 z 
                      gtest)


#add_subdirectory(ws)

# add_subdirectory(xratelimit)
