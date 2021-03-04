cmake_minimum_required(VERSION 3.8)


aux_source_directory(./ xrpc_ratelimit)
add_executable(xratelimit_test ${xrpc_ratelimit})

#add_dependencies(xratelimit_test xrpc xxbase)

target_link_libraries(xratelimit_test PRIVATE xrpc trezor-crypto xxbase)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(xratelimit_test PRIVATE gtest)
endif()

