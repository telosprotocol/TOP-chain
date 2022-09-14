cmake_minimum_required(VERSION 3.8)

add_executable(xquic_node cs_mini.cpp xquic_node.cpp)

target_include_directories(xquic_node PRIVATE ${PROJECT_SOURCE_DIR}/src/xtopcom/xdepends/xquic/include)

# target_link_libraries(xquic_node xquic -levent -lm)
set(SSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../src/xtopcom/xdepends/boringssl_static_libs/libssl.a;${CMAKE_CURRENT_SOURCE_DIR}/../../src/xtopcom/xdepends/boringssl_static_libs/libcrypto.a)
target_link_libraries(xquic_node xquic-static ${SSL_PATH} -levent -lm -pthread)

if(LEAK_TRACER)
    target_link_libraries(xquic_node leaktracer -ldl)
endif()