cmake_minimum_required(VERSION 3.8)

message("========== thrid-party BEGIN ============")

if(XBUILD_TEST)
    add_subdirectory(googletest)
endif()

if (XUSE_JEMALLOC)
    message(STATUS "")
    message(STATUS "========== thrid-party: jemalloc ============")

    execute_process(
        COMMAND bash "-c" "./autogen.sh"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc
    )

    execute_process(
        COMMAND make
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc
        ERROR_FILE "make_jemalloc.log"
        OUTPUT_FILE "make_jemalloc.log"
    )
endif()

message(STATUS "")
message(STATUS "========== thrid-party: xquic ============")
# options for compile xquic
add_definitions(-DBORINGSSL_PREFIX=FORQUIC)
# include_directories(./boringssl_static_libs/) # to find boringssl_prefix_symbols.h 
# add_subdirectory(boringssl)
set(SSL_TYPE "boringssl")
set(SSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boringssl)
set(SSL_INC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boringssl/include ${CMAKE_CURRENT_SOURCE_DIR}/boringssl_static_libs)
# if compile boringssl, find static lib here:
# set(SSL_LIB_PATH ${LIBRARY_OUTPUT_PATH}/libssl.a;${LIBRARY_OUTPUT_PATH}/libcrypto.a)
# for now we put it here:
set(SSL_LIB_PATH ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/boringssl_static_libs/libbrssl.a;${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/boringssl_static_libs/libbrcrypto.a)

add_subdirectory(xquic)


message(STATUS "")
message(STATUS "========== thrid-party: libevent ============")
SET(EVENT__LIBRARY_TYPE STATIC CACHE STRING "Build libevent only static")
SET(EVENT__DISABLE_OPENSSL ON CACHE BOOL "Build libevent library without openssl")
SET(EVENT__DISABLE_BENCHMARK ON CACHE BOOL "Build libevent disable benchmark")
SET(EVENT__DISABLE_TESTS ON CACHE BOOL "Build libevent disable test")
SET(EVENT__DISABLE_REGRESS ON CACHE BOOL "Build libevent disable regress test")
SET(EVENT__DISABLE_SAMPLES ON CACHE BOOL "Build libevent disable samples")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
SET(EVENT__DISABLE_DEBUG_MODE OFF CACHE BOOL "Build libevent enable debug")
else ()
SET(EVENT__DISABLE_DEBUG_MODE ON CACHE BOOL "Build libevent disable debug")
endif ()

add_subdirectory(libevent)


message("========== thrid-party END ============")

