cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xrpc_ws)
add_executable(xws_test ${xrpc_ws})

#add_dependencies(xws_test xrpc xchaininit xxbase)

target_link_libraries(xws_test PRIVATE xrpc xvnetwork xchaininit xxbase gtest)
