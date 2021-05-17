cmake_minimum_required(VERSION 3.8)

#compile definitions
add_definitions(
    -DUSE_STANDALONE_ASIO
    -DASIO_STANDALONE
)

aux_source_directory(./src xelect_net_src)
aux_source_directory(./proto xelect_net_proto)
add_library(xelect_net STATIC ${xelect_net_src} ${xelect_net_proto})

#add_dependencies(xelect_net xwrouter xkad xpbase xelect_common  xcommon xbasic https_client)
target_link_libraries(xelect_net PRIVATE xwrouter xkad xpbase xgossip xtransport xcommon xnetwork xbasic cpp_redis tacopie xelect_common https_client)

if (XENABLE_TESTS)
    add_subdirectory(demo)
endif()
