cmake_minimum_required(VERSION 3.8)

if(XENABLE_TESTS)
    add_subdirectory(googletest)
endif()


# add_subdirectory(boringssl)
set(SSL_TYPE "boringssl")
set(SSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boringssl)
set(SSL_INC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boringssl/include)
# if compile boringssl, find static lib here:
# set(SSL_LIB_PATH ${LIBRARY_OUTPUT_PATH}/libssl.a;${LIBRARY_OUTPUT_PATH}/libcrypto.a)
# for now we put it here:
set(SSL_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boringssl_static_libs/libssl.a;${CMAKE_CURRENT_SOURCE_DIR}/boringssl_static_libs/libcrypto.a)

add_subdirectory(xquic)

