file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_DIRECTORY})

message("========== thrid-party BEGIN ============")

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
        COMMAND cat "${LZ4_MAKE_LOG}"
        WORKING_DIRECTORY ${LZ4_WORKING_DIRECTORY}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(FATAL_ERROR "execute make failed")
else()
    message(STATUS "execute make done")
endif()

execute_process(
    COMMAND rm "-f" "${LZ4_MAKE_LOG}"
    WORKING_DIRECTORY ${LZ4_WORKING_DIRECTORY}
    ERROR_FILE ${LZ4_MAKE_LOG}
    OUTPUT_FILE ${LZ4_MAKE_LOG}
)

message(STATUS "executing 'cp liblz4.a ${LIBRARY_OUTPUT_DIRECTORY}'")
execute_process(
    COMMAND cp "lib/liblz4.a" "${LIBRARY_OUTPUT_DIRECTORY}"
    WORKING_DIRECTORY ${LZ4_WORKING_DIRECTORY}
    RESULT_VARIABLE Lz4ConfigResult
)
if (${Lz4ConfigResult})
    message(FATAL_ERROR "execute 'cp liblz4.a' failed")
else()
    message(STATUS "execute 'cp liblz4.a' done")
endif()


message(STATUS "")
message(STATUS "========== thrid-party: xquic ============")
# options for compile xquic
add_definitions(-DBORINGSSL_PREFIX=FORQUIC)
# include_directories(./boringssl_static_libs/) # to find boringssl_prefix_symbols.h
# add_subdirectory(boringssl)
set(SSL_TYPE "boringssl")
set(SSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boringssl)
set(SSL_INC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boringssl/include ${CMAKE_CURRENT_SOURCE_DIR}/prebuild/boringssl/include)
# if compile boringssl, find static lib here:
# set(SSL_LIB_PATH ${LIBRARY_OUTPUT_PATH}/libssl.a;${LIBRARY_OUTPUT_PATH}/libcrypto.a)
# for now we put it here:
set(SSL_LIB_PATH ${CMAKE_SOURCE_DIR}/third_party/prebuild/libs/${XSYSTEM_NAME_LOWERCASE}/libbrssl.a;${CMAKE_SOURCE_DIR}/third_party/prebuild/libs/${XSYSTEM_NAME_LOWERCASE}/libbrcrypto.a)
set(XQC_ENABLE_TESTING OFF)
add_compile_options(-Wno-stringop-truncation -Wno-array-parameter)
add_subdirectory(xquic)

message(STATUS "")
message(STATUS "========== thrid-party: jsoncpp ============")

set(JSONCPP_SETUP_LOG "jsoncpp_setup.log")
set(JSONCPP_COMPILE_LOG "jsoncpp_compile.log")
set(JSONCPP_SUBMODULE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp)
set(JSONCPP_BUILD_FOLDER_NAME build)
set(JSONCPP_BUILD_DIRECTORY ${JSONCPP_SUBMODULE_DIRECTORY}/${JSONCPP_BUILD_FOLDER_NAME})

message(STATUS "executing 'meson setup'")
execute_process(
    COMMAND bash "-c" "meson setup --default-library=static ${JSONCPP_BUILD_FOLDER_NAME}"
    WORKING_DIRECTORY ${JSONCPP_SUBMODULE_DIRECTORY}
    ERROR_FILE ${JSONCPP_SETUP_LOG}
    OUTPUT_FILE ${JSONCPP_SETUP_LOG}
    RESULT_VARIABLE JsonCppMesonResult
)
if (${JsonCppMesonResult})
    execute_process(
        COMMAND bash "-c" "cat ${JSONCPP_SETUP_LOG}"
        WORKING_DIRECTORY ${JSONCPP_SUBMODULE_DIRECTORY}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(FATAL_ERROR "execute 'meson setup' failed")
else()
    message(STATUS "execute 'meson setup' done")
endif()

message(STATUS "executing 'meson compile'")
execute_process(
    COMMAND bash "-c" "meson compile"
    WORKING_DIRECTORY ${JSONCPP_BUILD_DIRECTORY}
    ERROR_FILE ${JSONCPP_COMPILE_LOG}
    OUTPUT_FILE ${JSONCPP_COMPILE_LOG}
    RESULT_VARIABLE JsonCppMesonResult
)
if (${JsonCppMesonResult})
    execute_process(
        COMMAND bash "-c" "cat ${JSONCPP_COMPILE_LOG}"
        WORKING_DIRECTORY ${JSONCPP_BUILD_DIRECTORY}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(FATAL_ERROR "execute 'meson compile' failed")
else()
    message(STATUS "execute 'meson compile' done")
endif()

message(STATUS "executing 'cp libjsoncpp.a ${LIBRARY_OUTPUT_DIRECTORY}'")
execute_process(
    COMMAND bash "-c" "cp libjsoncpp.a ${LIBRARY_OUTPUT_DIRECTORY}"
    WORKING_DIRECTORY ${JSONCPP_BUILD_DIRECTORY}
    RESULT_VARIABLE JsonCppCpResult
)
if (${JsonCppCpResult})
    message(FATAL_ERROR "execute 'cp libjsoncpp.a' failed")
else()
    message(STATUS "execute 'cp libjsoncpp.a' done")
endif()

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
set(CROSSGUID_TESTS OFF CACHE BOOL "disable CROSSGUID_TESTS")
add_subdirectory(crossguid)

message("========== thrid-party END ============")

