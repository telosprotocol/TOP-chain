cmake_minimum_required(VERSION 3.8)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/proto)

aux_source_directory(./src xtransport_src)
aux_source_directory(./proto proto_src)
aux_source_directory(./xquic_node xquic_node_src)

add_library(xtransport ${xtransport_src} ${proto_src} ${xquic_node_src})

target_include_directories(xtransport PUBLIC ${PROJECT_SOURCE_DIR}/src/xtopcom/xdepends/xquic/include)

set(SSL_PATH ${PROJECT_SOURCE_DIR}/src/xtopcom/xdepends/boringssl_static_libs/libssl.a;${PROJECT_SOURCE_DIR}/src/xtopcom/xdepends/boringssl_static_libs/libcrypto.a)

#add_dependencies(xtransport xpbase xutility xxbase)
target_link_libraries(xtransport xpbase xutility xxbase protobuf jsoncpp xcrypto xquic-static ${SSL_PATH} -levent -lm -pthread)

if (BUILD_METRICS)
    #add_dependencies(xtransport xmetrics)
    target_link_libraries(xtransport xmetrics)
endif()
