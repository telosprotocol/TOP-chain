cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xgrpc_mgr_src)
add_compile_options(-Wall)
add_compile_options(-Werror)
add_library(xgrpc_mgr ${xgrpc_mgr_src})

find_library(gRPC_LIBRARIES NAMES libgrpc.a libgrpc++_unsecure.a libgrpc_unsecure.a libgpr.a libaddress_sorting.a)
find_library(ares_LIBRARIES libcares.a REQUIRED)

target_link_libraries(xgrpc_mgr PRIVATE xrpc xgrpcservice ${gRPC_LIBRARIES} ${ares_LIBRARIES})

if (BUILD_METRICS)
    #add_dependencies(xgrpc_mgr xmetrics)
    target_link_libraries(xgrpc_mgr PRIVATE xmetrics)
endif()
