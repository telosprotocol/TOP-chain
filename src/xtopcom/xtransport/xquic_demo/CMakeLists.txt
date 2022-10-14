cmake_minimum_required(VERSION 3.8)

add_executable(xquic_node cs_mini.cpp xquic_node.cpp)

target_include_directories(xquic_node PRIVATE ${PROJECT_SOURCE_DIR}/src/xtopcom/xdepends/xquic/include)

set(SSL_PATH ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/boringssl_static_libs/libbrssl.a;${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/boringssl_static_libs/libbrcrypto.a)

target_link_libraries(xquic_node PRIVATE xquic-static xbasic ${SSL_PATH} -lm -pthread)

add_dependencies(xquic_node event_static)
target_include_directories(xquic_node PRIVATE ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/libevent/include/) # for libevent header file
target_include_directories(xquic_node PRIVATE ${CMAKE_BINARY_DIR}/src/xtopcom/xdepends/libevent/include/) # for event-config.h
target_link_libraries(xquic_node PRIVATE ${CMAKE_BINARY_DIR}/src/xtopcom/xdepends/libevent/lib/libevent.a) # static library output path
target_link_libraries(xquic_node PRIVATE ${CMAKE_BINARY_DIR}/src/xtopcom/xdepends/libevent/lib/libevent_pthreads.a) # for event pthread

if(LEAK_TRACER)
    target_link_libraries(xquic_node leaktracer -ldl)
endif()