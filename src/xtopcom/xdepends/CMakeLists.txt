cmake_minimum_required(VERSION 3.22)

# SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

message("========== thrid-party BEGIN ============")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    cmake_host_system_information(RESULT ID QUERY DISTRIB_ID)
endif()
if(XBUILD_TEST)
    if (NOT ID STREQUAL "ubuntu")
        add_subdirectory(googletest)
    endif()
endif()

file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_DIRECTORY})

if (XUSE_JEMALLOC)
    message(STATUS "")
    message(STATUS "========== thrid-party: jemalloc ============")

    set(JEMALLOC_AUTOGEN_LOG "autogen_jemalloc.log")
    set(JEMALLOC_MAKE_LOG "make_jemalloc.log")
    set(JEMALLOC_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc)

    execute_process(
        COMMAND bash "-c" "rm ${JEMALLOC_AUTOGEN_LOG} ${JEMALLOC_MAKE_LOG}"
        WORKING_DIRECTORY ${JEMALLOC_WORKING_DIRECTORY}
        ERROR_FILE "/dev/null"
        OUTPUT_FILE "/dev/null"
    )
    # execute_process(
    #     COMMAND bash "-c" "dos2unix * */* */*/* */*/*/*"
    #     WORKING_DIRECTORY ${JEMALLOC_WORKING_DIRECTORY}
    #     ERROR_FILE "/dev/null"
    #     OUTPUT_FILE "/dev/null"
    # )

    message(STATUS "executing autogen.sh")
    execute_process(
        COMMAND bash "-c" "./autogen.sh"
        WORKING_DIRECTORY ${JEMALLOC_WORKING_DIRECTORY}
        ERROR_FILE ${JEMALLOC_AUTOGEN_LOG}
        OUTPUT_FILE ${JEMALLOC_AUTOGEN_LOG}
        RESULT_VARIABLE JemallocConfigResult
    )
    if (${JemallocConfigResult})
        execute_process(
            COMMAND bash "-c" "cat ${JEMALLOC_AUTOGEN_LOG}"
            WORKING_DIRECTORY ${JEMALLOC_WORKING_DIRECTORY}
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(FATAL_ERROR "execute autogen.sh failed")
    else()
        message(STATUS "execute autogen.sh done")
    endif()

    message(STATUS "executing make")
    execute_process(
        COMMAND make
        WORKING_DIRECTORY ${JEMALLOC_WORKING_DIRECTORY}
        ERROR_FILE ${JEMALLOC_MAKE_LOG}
        OUTPUT_FILE ${JEMALLOC_MAKE_LOG}
        RESULT_VARIABLE JemallocConfigResult
    )
    if (${JemallocConfigResult})
        execute_process(
            COMMAND bash "-c" "cat ${JEMALLOC_MAKE_LOG}"
            WORKING_DIRECTORY ${JEMALLOC_WORKING_DIRECTORY}
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(FATAL_ERROR "execute make failed")
    else()
        message(STATUS "execute make done")
    endif()
elseif(BUILD_GPERF OR BUILD_GHPERF OR TCMALLOC)
    message(STATUS "")
    message(STATUS "========== thrid-party: tcmalloc ============")

    set(GPERFTOOLS_AUTOGEN_LOG "autogen_gperftools.log")
    set(GPERFTOOLS_CONFIGURE_LOG "configure_gperftools.log")
    set(GPERFTOOLS_MAKE_LOG "make_gperftools.log")
    set(GPERFTOOLS_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gperftools)

    execute_process(
        COMMAND bash "-c" "rm ${GPERFTOOLS_AUTOGEN_LOG} ${GPERFTOOLS_CONFIGURE_LOG} ${GPERFTOOLS_MAKE_LOG}"
        WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
        ERROR_FILE "/dev/null"
        OUTPUT_FILE "/dev/null"
    )
    # execute_process(
    #     COMMAND bash "-c" "dos2unix * */* */*/*"
    #     WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
    #     ERROR_FILE "/dev/null"
    #     OUTPUT_FILE "/dev/null"
    # )

    message(STATUS "executing autogen.sh")
    execute_process(
        COMMAND bash "-c" "./autogen.sh"
        WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
        ERROR_FILE ${GPERFTOOLS_AUTOGEN_LOG}
        OUTPUT_FILE ${GPERFTOOLS_AUTOGEN_LOG}
        RESULT_VARIABLE GperfToolsConfigResult
    )
    if (${GperfToolsConfigResult})
        execute_process(
            COMMAND bash "-c" "cat ${GPERFTOOLS_AUTOGEN_LOG}"
            WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(FATAL_ERROR "execute autogen.sh failed")
    else()
        message(STATUS "execute autogen.sh done")
    endif()

    message(STATUS "updating ltmain.sh")
    execute_process(
        COMMAND bash "-c" "mv ltmain.sh ltmain.origin"
        WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
    )
    set(PATTERN "\"$objname\"")
    set(REPLACEMENT "\"$objdir/$objname\"")
    execute_process(
        COMMAND sed "-e" "2281 s@${PATTERN}@${REPLACEMENT}@" "ltmain.origin"
        WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
        ERROR_FILE "ltmain.sh"
        OUTPUT_FILE "ltmain.sh"
        RESULT_VARIABLE GperfToolsConfigResult
    )
    if (${GperfToolsConfigResult})
        execute_process(
            COMMAND bash "-c" "cat ltmain.sh"
            WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(FATAL_ERROR "update ltmain.sh failed")
    else()
        message(STATUS "update ltmain.sh done")
    endif()

    message(STATUS "executing configure")
    execute_process(
        COMMAND bash "-c" "./configure"
        WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
        ERROR_FILE ${GPERFTOOLS_CONFIGURE_LOG}
        OUTPUT_FILE ${GPERFTOOLS_CONFIGURE_LOG}
        RESULT_VARIABLE GperfToolsConfigResult
    )
    if (${GperfToolsConfigResult})
        execute_process(
            COMMAND bash "-c" "cat ${GPERFTOOLS_CONFIGURE_LOG}"
            WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(FATAL_ERROR "execute configure failed")
    else()
        message(STATUS "execute configure done")
    endif()

    message(STATUS "executing make")
    execute_process(
        COMMAND make
        WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
        ERROR_FILE ${GPERFTOOLS_MAKE_LOG}
        OUTPUT_FILE ${GPERFTOOLS_MAKE_LOG}
        RESULT_VARIABLE GperfToolsConfigResult
    )
    if (${GperfToolsConfigResult})
        execute_process(
            COMMAND bash "-c" "cat ${GPERFTOOLS_MAKE_LOG}"
            WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(FATAL_ERROR "execute make failed")
    else()
        execute_process(
            COMMAND bash "-c" "rm ./.libs/libtcmalloc_and_profiler.so*"
            WORKING_DIRECTORY ${GPERFTOOLS_WORKING_DIRECTORY}
            ERROR_FILE "/dev/null"
            OUTPUT_FILE "/dev/null"
        )
        message(STATUS "execute make done")
    endif()
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

add_compile_options(-Wno-stringop-truncation -Wno-array-parameter)
add_subdirectory(xquic)

if (NOT ID STREQUAL "ubuntu")
message(STATUS "")
message(STATUS "========== thrid-party: libevent ============")
set(EVENT__LIBRARY_TYPE STATIC CACHE STRING "Build libevent only static")
set(EVENT__DISABLE_OPENSSL ON CACHE BOOL "Build libevent library without openssl")
set(EVENT__DISABLE_BENCHMARK ON CACHE BOOL "Build libevent disable benchmark")
set(EVENT__DISABLE_TESTS ON CACHE BOOL "Build libevent disable test")
set(EVENT__DISABLE_REGRESS ON CACHE BOOL "Build libevent disable regress test")
set(EVENT__DISABLE_SAMPLES ON CACHE BOOL "Build libevent disable samples")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
set(EVENT__DISABLE_DEBUG_MODE OFF CACHE BOOL "Build libevent enable debug")
else ()
set(EVENT__DISABLE_DEBUG_MODE ON CACHE BOOL "Build libevent disable debug")
endif ()

add_subdirectory(libevent)
endif()

message(STATUS "")
message(STATUS "========== thrid-party: lz4 ============")

set(LZ4_MAKE_LOG "make_lz4.log")
set(LZ4_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lz4)

message(STATUS "executing make")
execute_process(
    COMMAND make
    WORKING_DIRECTORY ${LZ4_WORKING_DIRECTORY}
    ERROR_FILE ${LZ4_MAKE_LOG}
    OUTPUT_FILE ${LZ4_MAKE_LOG}
    RESULT_VARIABLE Lz4ConfigResult
)
if (${Lz4ConfigResult})
    execute_process(
        COMMAND bash "-c" "cat ${LZ4_MAKE_LOG}"
        WORKING_DIRECTORY ${LZ4_WORKING_DIRECTORY}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(FATAL_ERROR "execute make failed")
else()
    message(STATUS "execute make done")
endif()

message(STATUS "executing 'cp liblz4.a ${LIBRARY_OUTPUT_DIRECTORY}'")
execute_process(
    COMMAND bash "-c" "cp lib/liblz4.a ${LIBRARY_OUTPUT_DIRECTORY}"
    WORKING_DIRECTORY ${LZ4_WORKING_DIRECTORY}
    RESULT_VARIABLE Lz4ConfigResult
)
if (${Lz4ConfigResult})
    message(FATAL_ERROR "execute 'cp liblz4.a' failed")
else()
    message(STATUS "execute 'cp liblz4.a' done")
endif()

# message(STATUS "")
# message(STATUS "========== thrid-party: rocksdb ============")
# if (XUSE_JEMALLOC)
#     set(WITH_JEMALLOC ON)
# endif()
# set(PORTABLE ON CACHE BOOL "rocksdb enable PORTABLE")
# set(ROCKSDB_BUILD_SHARED OFF CACHE BOOL "rocksdb disable ROCKSDB_BUILD_SHARED")
# set(WITH_TESTS OFF CACHE BOOL "rocksdb disable WITH_TESTS")
# set(WITH_ALL_TESTS OFF CACHE BOOL "rocksdb disable WITH_ALL_TESTS")
# set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "rocksdb disable WITH_BENCHMARK_TOOLS")
# set(WITH_CORE_TOOLS OFF CACHE BOOL "rocksdb disable WITH_CORE_TOOLS")
# set(WITH_TRACE_TOOLS OFF CACHE BOOL "rocksdb disable WITH_TRACE_TOOLS")
# set(WITH_TOOLS OFF CACHE BOOL "rocksdb disable WITH_TOOLS")
# set(WITH_SNAPPY ON CACHE BOOL "rocksdb enable WITH_SNAPPY")
# set(WITH_LZ4 ON CACHE BOOL "rocksdb enable WITH_LZ4")
# set(WITH_ZLIB ON CACHE BOOL "rocksdb enable WITH_ZLIB")
# set(WITH_ZSTD ON CACHE BOOL "rocksdb enable WITH_ZSTD")

# add_compile_options(-Wno-redundant-move -Wno-maybe-uninitialized)
# add_subdirectory(rocksdb)

# message(STATUS "")
# message(STATUS "========== thrid-party: jsoncpp ============")

# set(JSONCPP_SETUP_LOG "jsoncpp_setup.log")
# set(JSONCPP_COMPILE_LOG "jsoncpp_compile.log")
# set(JSONCPP_SUBMODULE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp)
# set(JSONCPP_BUILD_FOLDER_NAME build)
# set(JSONCPP_BUILD_DIRECTORY ${JSONCPP_SUBMODULE_DIRECTORY}/${JSONCPP_BUILD_FOLDER_NAME})

# message(STATUS "executing 'meson setup'")
# execute_process(
#     COMMAND bash "-c" "meson setup --default-library=static ${JSONCPP_BUILD_FOLDER_NAME}"
#     WORKING_DIRECTORY ${JSONCPP_SUBMODULE_DIRECTORY}
#     ERROR_FILE ${JSONCPP_SETUP_LOG}
#     OUTPUT_FILE ${JSONCPP_SETUP_LOG}
#     RESULT_VARIABLE JsonCppMesonResult
# )
# if (${JsonCppMesonResult})
#     execute_process(
#         COMMAND bash "-c" "cat ${JSONCPP_SETUP_LOG}"
#         WORKING_DIRECTORY ${JSONCPP_SUBMODULE_DIRECTORY}
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#     )
#     message(FATAL_ERROR "execute 'meson setup' failed")
# else()
#     message(STATUS "execute 'meson setup' done")
# endif()

# message(STATUS "executing 'meson compile'")
# execute_process(
#     COMMAND bash "-c" "meson compile"
#     WORKING_DIRECTORY ${JSONCPP_BUILD_DIRECTORY}
#     ERROR_FILE ${JSONCPP_COMPILE_LOG}
#     OUTPUT_FILE ${JSONCPP_COMPILE_LOG}
#     RESULT_VARIABLE JsonCppMesonResult
# )
# if (${JsonCppMesonResult})
#     execute_process(
#         COMMAND bash "-c" "cat ${JSONCPP_COMPILE_LOG}"
#         WORKING_DIRECTORY ${JSONCPP_BUILD_DIRECTORY}
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#     )
#     message(FATAL_ERROR "execute 'meson compile' failed")
# else()
#     message(STATUS "execute 'meson compile' done")
# endif()

# message(STATUS "executing 'cp libjsoncpp.a ${LIBRARY_OUTPUT_DIRECTORY}'")
# execute_process(
#     COMMAND bash "-c" "cp libjsoncpp.a ${LIBRARY_OUTPUT_DIRECTORY}"
#     WORKING_DIRECTORY ${JSONCPP_BUILD_DIRECTORY}
#     RESULT_VARIABLE JsonCppCpResult
# )
# if (${JsonCppCpResult})
#     message(FATAL_ERROR "execute 'cp libjsoncpp.a' failed")
# else()
#     message(STATUS "execute 'cp libjsoncpp.a' done")
# endif()

# message(STATUS "")
# message(STATUS "========== thrid-party: secp256k1 ============")

# set(SECP256K1_AUTOGEN_LOG "autogen_secp256k1.log")
# set(SECP256K1_CONFIGURE_LOG "configure_secp256k1.log")
# set(SECP256K1_MAKE_LOG "make_secp256k1.log")
# set(SECP256K1_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1)
# set(SECP256k1_INCLUDE_DIRECTORY ${SECP256K1_WORKING_DIRECTORY}/include)

# file(MAKE_DIRECTORY ${SECP256k1_INCLUDE_DIRECTORY}/secp256k1)
# execute_process(
#     COMMAND bash "-c" "cp *.h secp256k1/"
#     WORKING_DIRECTORY ${SECP256k1_INCLUDE_DIRECTORY}
#     ERROR_FILE "/dev/null"
#     OUTPUT_FILE "/dev/null"
# )

# execute_process(
#     COMMAND bash "-c" "rm ${SECP256K1_AUTOGEN_LOG} ${SECP256K1_CONFIGURE_LOG} ${SECP256K1_MAKE_LOG}"
#     WORKING_DIRECTORY ${SECP256K1_WORKING_DIRECTORY}
#     ERROR_FILE "/dev/null"
#     OUTPUT_FILE "/dev/null"
# )
# execute_process(
#     COMMAND bash "-c" "dos2unix * */* */*/*"
#     WORKING_DIRECTORY ${SECP256K1_WORKING_DIRECTORY}
#     ERROR_FILE "/dev/null"
#     OUTPUT_FILE "/dev/null"
# )

# message(STATUS "executing autogen.sh")
# execute_process(
#     COMMAND bash "-c" "./autogen.sh"
#     WORKING_DIRECTORY ${SECP256K1_WORKING_DIRECTORY}
#     ERROR_FILE ${SECP256K1_AUTOGEN_LOG}
#     OUTPUT_FILE ${SECP256K1_AUTOGEN_LOG}
#     RESULT_VARIABLE Secp256k1ConfigResult
# )
# if (${Secp256k1ConfigResult})
#     execute_process(
#         COMMAND bash "-c" "cat ${SECP256K1_AUTOGEN_LOG}"
#         WORKING_DIRECTORY ${SECP256K1_WORKING_DIRECTORY}
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#     )
#     message(FATAL_ERROR "execute autogen.sh failed")
# else()
#     message(STATUS "execute autogen.sh done")
# endif()

# message(STATUS "executing configure")
# execute_process(
#     COMMAND bash "-c" "./configure --disable-benchmark --disable-tests --enable-module-recovery"
#     WORKING_DIRECTORY ${SECP256K1_WORKING_DIRECTORY}
#     ERROR_FILE ${SECP256K1_CONFIGURE_LOG}
#     OUTPUT_FILE ${SECP256K1_CONFIGURE_LOG}
#     RESULT_VARIABLE Secp256k1ConfigResult
# )
# if (${Secp256k1ConfigResult})
#     execute_process(
#         COMMAND bash "-c" "cat ${SECP256K1_CONFIGURE_LOG}"
#         WORKING_DIRECTORY ${SECP256K1_WORKING_DIRECTORY}
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#     )
#     message(FATAL_ERROR "execute configure failed")
# else()
#     message(STATUS "execute configure done")
# endif()

# message(STATUS "executing make")
# execute_process(
#     COMMAND make
#     WORKING_DIRECTORY ${SECP256K1_WORKING_DIRECTORY}
#     ERROR_FILE ${SECP256K1_MAKE_LOG}
#     OUTPUT_FILE ${SECP256K1_MAKE_LOG}
#     RESULT_VARIABLE Secp256k1ConfigResult
# )
# if (${Secp256k1ConfigResult})
#     execute_process(
#         COMMAND bash "-c" "cat ${SECP256K1_MAKE_LOG}"
#         WORKING_DIRECTORY ${SECP256K1_WORKING_DIRECTORY}
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#     )
#     message(FATAL_ERROR "execute make failed")
# else()
#     message(STATUS "execute make done")
# endif()


# message(STATUS "")
# message(STATUS "========== thrid-party: openssl ============")

# set(OPENSSL_CONFIGURE_LOG "configure_openssl.log")
# set(OPENSSL_MAKE_LOG "make_openssl.log")
# set(OPENSSL_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/openssl)

# execute_process(
#     COMMAND bash "-c" "rm ${OPENSSL_CONFIGURE_LOG} ${OPENSSL_MAKE_LOG}"
#     WORKING_DIRECTORY ${OPENSSL_WORKING_DIRECTORY}
#     ERROR_FILE "/dev/null"
#     OUTPUT_FILE "/dev/null"
# )
# execute_process(
#     COMMAND bash "-c" "dos2unix *"
#     WORKING_DIRECTORY ${OPENSSL_WORKING_DIRECTORY}
#     ERROR_FILE "/dev/null"
#     OUTPUT_FILE "/dev/null"
# )

# message(STATUS "executing Configure")
# execute_process(
#     COMMAND bash "-c" "./Configure -static no-shared linux-x86_64"
#     WORKING_DIRECTORY ${OPENSSL_WORKING_DIRECTORY}
#     ERROR_FILE ${OPENSSL_CONFIGURE_LOG}
#     OUTPUT_FILE ${OPENSSL_CONFIGURE_LOG}
#     RESULT_VARIABLE OpensslConfigResult
# )
# if (${OpensslConfigResult})
#     execute_process(
#         COMMAND bash "-c" "cat ${OPENSSL_CONFIGURE_LOG}"
#         WORKING_DIRECTORY ${OPENSSL_WORKING_DIRECTORY}
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#     )
#     message(FATAL_ERROR "execute Configure failed")
# else()
#     message(STATUS "execute Configure done")
# endif()

# message(STATUS "executing make")
# execute_process(
#     COMMAND make
#     WORKING_DIRECTORY ${OPENSSL_WORKING_DIRECTORY}
#     ERROR_FILE ${OPENSSL_MAKE_LOG}
#     OUTPUT_FILE ${OPENSSL_MAKE_LOG}
#     RESULT_VARIABLE OpensslConfigResult
# )
# if (${OpensslConfigResult})
#     execute_process(
#         COMMAND bash "-c" "cat ${OPENSSL_MAKE_LOG}"
#         WORKING_DIRECTORY ${OPENSSL_WORKING_DIRECTORY}
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#     )
#     message(FATAL_ERROR "execute make failed")
# else()
#     message(STATUS "execute make done")
# endif()

# message(STATUS "executing 'cp libssl.a ${LIBRARY_OUTPUT_DIRECTORY}'")
# execute_process(
#     COMMAND bash "-c" "cp libssl.a ${LIBRARY_OUTPUT_DIRECTORY}"
#     WORKING_DIRECTORY ${OPENSSL_WORKING_DIRECTORY}
#     RESULT_VARIABLE OpensslConfigResult
# )
# if (${OpensslConfigResult})
#     message(FATAL_ERROR "execute 'cp libssl.a' failed")
# else()
#     message(STATUS "execute 'cp libssl.a' done")
# endif()

# message(STATUS "executing 'cp libcrypto.a ${LIBRARY_OUTPUT_DIRECTORY}'")
# execute_process(
#     COMMAND bash "-c" "cp libcrypto.a ${LIBRARY_OUTPUT_DIRECTORY}"
#     WORKING_DIRECTORY ${OPENSSL_WORKING_DIRECTORY}
#     RESULT_VARIABLE OpensslConfigResult
# )
# if (${OpensslConfigResult})
#     message(FATAL_ERROR "execute 'cp libcrypto.a' failed")
# else()
#     message(STATUS "execute 'cp libcrypto.a' done")
# endif()

# message(STATUS "")
# message(STATUS "========== thrid-party: crypto++ ============")

# set(CRYPTOPP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cryptopp)
# add_subdirectory(cryptopp-cmake)

# message(STATUS "")
# message(STATUS "========== thrid-party: protobuf ============")

# set(PROTOBUF_AUTOGEN_LOG "autogen_protobuf.log")
# set(PROTOBUF_CONFIGURE_LOG "configure_protobuf.log")
# set(PROTOBUF_MAKE_LOG "make_protobuf.log")
# set(PROTOBUF_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/protobuf)

# execute_process(
#     COMMAND bash "-c" "rm ${PROTOBUF_AUTOGEN_LOG} ${PROTOBUF_CONFIGURE_LOG} ${PROTOBUF_MAKE_LOG}"
#     WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#     ERROR_FILE "/dev/null"
#     OUTPUT_FILE "/dev/null"
# )
# execute_process(
#     COMMAND bash "-c" "find . -name \\*.sh | xargs dos2unix"
#     WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#     ERROR_FILE "/dev/null"
#     OUTPUT_FILE "/dev/null"
# )
# execute_process(
#     COMMAND bash "-c" "find . -name \\*.m4 | xargs dos2unix"
#     WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#     ERROR_FILE "/dev/null"
#     OUTPUT_FILE "/dev/null"
# )
# execute_process(
#     COMMAND bash "-c" "find . -name \\*.ac | xargs dos2unix"
#     WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#     ERROR_FILE "/dev/null"
#     OUTPUT_FILE "/dev/null"
# )
# execute_process(
#     COMMAND bash "-c" "find . -name \\*.am | xargs dos2unix"
#     WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#     ERROR_FILE "/dev/null"
#     OUTPUT_FILE "/dev/null"
# )

# message(STATUS "executing autogen.sh")
# execute_process(
#     COMMAND bash "-c" "./autogen.sh"
#     WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#     ERROR_FILE ${PROTOBUF_AUTOGEN_LOG}
#     OUTPUT_FILE ${PROTOBUF_AUTOGEN_LOG}
#     RESULT_VARIABLE ProtobufConfigResult
# )
# if (${ProtobufConfigResult})
#     execute_process(
#         COMMAND bash "-c" "cat ${PROTOBUF_AUTOGEN_LOG}"
#         WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#     )
#     message(FATAL_ERROR "execute autogen.sh failed")
# else()
#     message(STATUS "execute autogen.sh done")
# endif()

# message(STATUS "executing configure")
# execute_process(
#     COMMAND bash "-c" "./configure  --disable-shared --enable-static --with-zlib --with-pic"
#     WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#     ERROR_FILE ${PROTOBUF_CONFIGURE_LOG}
#     OUTPUT_FILE ${PROTOBUF_CONFIGURE_LOG}
#     RESULT_VARIABLE ProtobufConfigResult
# )
# if (${ProtobufConfigResult})
#     execute_process(
#         COMMAND bash "-c" "cat ${PROTOBUF_CONFIGURE_LOG}"
#         WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#     )
#     message(FATAL_ERROR "execute configure failed")
# else()
#     message(STATUS "execute configure done")
# endif()

# message(STATUS "executing make")
# execute_process(
#     COMMAND make
#     WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#     ERROR_FILE ${PROTOBUF_MAKE_LOG}
#     OUTPUT_FILE ${PROTOBUF_MAKE_LOG}
#     RESULT_VARIABLE ProtobufConfigResult
# )
# if (${ProtobufConfigResult})
#     execute_process(
#         COMMAND bash "-c" "cat ${PROTOBUF_MAKE_LOG}"
#         WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#     )
#     message(FATAL_ERROR "execute make failed")
# else()
#     message(STATUS "execute make done")
# endif()

# message(STATUS "executing 'cp libprotobuf.a ${LIBRARY_OUTPUT_DIRECTORY}'")
# execute_process(
#     COMMAND bash "-c" "cp src/.libs/libprotobuf.a ${LIBRARY_OUTPUT_DIRECTORY}"
#     WORKING_DIRECTORY ${PROTOBUF_WORKING_DIRECTORY}
#     RESULT_VARIABLE ProtobufConfigResult
# )
# if (${ProtobufConfigResult})
#     message(FATAL_ERROR "execute 'cp libprotobuf.a' failed")
# else()
#     message(STATUS "execute 'cp libprotobuf.a' done")
# endif()

message(STATUS "")
message(STATUS "========== thrid-party: ethash ============")

set(ETHASH_CMAKE_LOG "cmake_ethash.log")
set(ETHASH_MAKE_LOG "make_ethash.log")
set(ETHASH_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ethash/build)

file(MAKE_DIRECTORY ${ETHASH_WORKING_DIRECTORY})
message(STATUS "executing cmake")
execute_process(
    COMMAND cmake ".."
    WORKING_DIRECTORY ${ETHASH_WORKING_DIRECTORY}
    ERROR_FILE ${ETHASH_CMAKE_LOG}
    OUTPUT_FILE ${ETHASH_CMAKE_LOG}
    RESULT_VARIABLE EthashConfigResult
)
if (${EthashConfigResult})
    execute_process(
        COMMAND bash "-c" "cat ${ETHASH_CMAKE_LOG}"
        WORKING_DIRECTORY ${ETHASH_WORKING_DIRECTORY}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(FATAL_ERROR "execute cmake failed")
else()
    message(STATUS "execute cmake done")
endif()

message(STATUS "executing make")
execute_process(
    COMMAND make
    WORKING_DIRECTORY ${ETHASH_WORKING_DIRECTORY}
    ERROR_FILE ${ETHASH_MAKE_LOG}
    OUTPUT_FILE ${ETHASH_MAKE_LOG}
    RESULT_VARIABLE EthashConfigResult
)
if (${EthashConfigResult})
    execute_process(
        COMMAND bash "-c" "cat ${ETHASH_MAKE_LOG}"
        WORKING_DIRECTORY ${ETHASH_WORKING_DIRECTORY}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(FATAL_ERROR "execute make failed")
else()
    message(STATUS "execute make done")
endif()

message(STATUS "executing 'cp libethash.a ${LIBRARY_OUTPUT_DIRECTORY}'")
execute_process(
    COMMAND bash "-c" "cp lib/ethash/libethash.a ${LIBRARY_OUTPUT_DIRECTORY}"
    WORKING_DIRECTORY ${ETHASH_WORKING_DIRECTORY}
    RESULT_VARIABLE EthashConfigResult
)
if (${EthashConfigResult})
    message(FATAL_ERROR "execute 'cp libethash.a' failed")
else()
    message(STATUS "execute 'cp libethash.a' done")
endif()

message(STATUS "executing 'cp libkeccak.a ${LIBRARY_OUTPUT_DIRECTORY}'")
execute_process(
    COMMAND bash "-c" "cp lib/keccak/libkeccak.a ${LIBRARY_OUTPUT_DIRECTORY}"
    WORKING_DIRECTORY ${ETHASH_WORKING_DIRECTORY}
    RESULT_VARIABLE EthashConfigResult
)
if (${EthashConfigResult})
    message(FATAL_ERROR "execute 'cp libkeccak.a' failed")
else()
    message(STATUS "execute 'cp libkeccak.a' done")
endif()

message(STATUS "")
message(STATUS "========== thrid-party: crossguid ============")
add_subdirectory(crossguid)

# message(STATUS "")
# message(STATUS "========== thrid-party: gRPC ============")
# set(gRPC_BUILD_CSHARP_EXT OFF CACHE BOOL "disable gRPC_BUILD_CSHARP_EXT")
# set(gRPC_BUILD_CODEGEN OFF CACHE BOOL "disable gRPC_BUILD_CODEGEN")
# add_subdirectory(grpc)

message("========== thrid-party END ============")

