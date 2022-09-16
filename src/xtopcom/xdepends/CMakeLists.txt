cmake_minimum_required(VERSION 3.8)

if(XENABLE_TESTS)
    add_subdirectory(googletest)
endif()


# already move to top cmake file
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

