cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xrpc_test_src)
# aux_source_directory(./ws xrpc_ws)
#add_executable(xrpc_test ${xrpc_test_src} ${xrpc_ws})
#add_executable(xrpc_test  ${xrpc_ws})
add_executable(xrpc_test ${xrpc_test_src})

# add_dependencies(xrpc_test xrpc xchaininit xxbase)

if (${XOS_ID} MATCHES "centos")
target_include_directories(xrpc_test PRIVATE ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/openssl_include/openssl/)
endif()
target_link_libraries(xrpc_test PRIVATE xrpc xvnetwork xtxstore xloader xgenesis xxbase xdb ssl crypto gtest)


#add_subdirectory(ws)

# add_subdirectory(xratelimit)
