message("========== thrid-party BEGIN ============")

if(XBUILD_TEST)
    add_subdirectory(googletest)
endif()

if (XUSE_JEMALLOC)
    message(STATUS "")
    message(STATUS "========== thrid-party: jemalloc ============")

    set(JEMALLOC_AUTOGEN_LOG "autogen_jemalloc.log")
    set(JEMALLOC_MAKE_LOG "make_jemalloc.log")
    set(JEMALLOC_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jemalloc)

    if (ENABLE_JEPROF)
        message(STATUS "enable jeprof")
        SET(JEMALLOC_ENABLE_JEPROF "--enable-prof")
    endif()
    
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
        COMMAND bash "-c" "./autogen.sh ${JEMALLOC_ENABLE_JEPROF}"
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

add_subdirectory(xquic)


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

message("========== thrid-party END ============")

