cmake_minimum_required(VERSION 3.8)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/proto)

aux_source_directory(./proto proto_src)
aux_source_directory(./src xtransport_src)

add_library(xtransport ${proto_src} ${xtransport_src})

# MUST use PRIVATE include
target_include_directories(xtransport PRIVATE ${PROJECT_SOURCE_DIR}/src/xtopcom/xdepends/xquic/include)

set(SSL_LIB_PATH ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/boringssl_static_libs/libbrssl.a;${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/boringssl_static_libs/libbrcrypto.a)

target_link_libraries(xtransport PRIVATE xquic-static ${SSL_LIB_PATH})
target_link_libraries(xtransport PUBLIC xpbase xutility xxbase protobuf jsoncpp xcrypto -levent -lm -pthread)

if (BUILD_METRICS)
    target_link_libraries(xtransport PUBLIC xmetrics)
endif()
