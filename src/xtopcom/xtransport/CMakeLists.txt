cmake_minimum_required(VERSION 3.8)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/proto)

aux_source_directory(./src xtransport_src)
aux_source_directory(./proto proto_src)
add_library(xtransport ${xtransport_src} ${proto_src})

# MUST use PRIVATE include
target_include_directories(xtransport PRIVATE ${PROJECT_SOURCE_DIR}/src/xtopcom/xdepends/xquic/include)

set(SSL_LIB_PATH ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/boringssl_static_libs/libbrssl.a;${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/boringssl_static_libs/libbrcrypto.a)
target_link_libraries(xtransport PRIVATE xquic-static ${SSL_LIB_PATH})
target_link_libraries(xtransport PUBLIC xpbase xutility xxbase protobuf jsoncpp xcrypto -lm -pthread)

add_dependencies(xtransport event_static)
target_include_directories(xtransport PRIVATE ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/libevent/include/) # for libevent header file
target_include_directories(xtransport PRIVATE ${CMAKE_BINARY_DIR}/src/xtopcom/xdepends/libevent/include/) # for event-config.h
target_link_libraries(xtransport PRIVATE ${CMAKE_BINARY_DIR}/src/xtopcom/xdepends/libevent/lib/libevent.a) # static library output path

if (BUILD_METRICS)
    target_link_libraries(xtransport PUBLIC xmetrics)
endif()
