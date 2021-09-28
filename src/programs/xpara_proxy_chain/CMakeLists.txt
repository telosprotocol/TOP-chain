cmake_minimum_required(VERSION 3.8)

# add_definitions(
#     -DUSE_STANDALONE_ASIO
#     -DASIO_STANDALONE
#     # -D__STRICT_ANSI__
#     -DRPC_BUILD
# )
# if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#     add_compile_options(-Wno-deprecated-declarations)
#     add_compile_options(-Wno-write-strings)
# elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#     add_compile_options(-Wno-writable-strings)
#     add_compile_options(-Wno-deprecated-declarations)
#     add_compile_options(-Wno-write-strings)
#     add_compile_options(-Wno-unused-variable)
#     add_compile_options(-Wno-unused-function)
# elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
#     add_definitions(-D_WIN32_WINNT=0x0501)
#     add_definitions(-DNOMINMAX)  # enable std::min/std::max
#     add_definitions(-D_CRT_SECURE_NO_WARNINGS) # disable strncpy -> strncpy_s warnings
#     add_compile_options(/Zc:__cplusplus) # using correct __cplusplus value
#     add_compile_options(/utf-8) # input/output as utf-8
# endif()

# set(LINK_ARGS secp256k1 trezor-crypto ssl crypto)
# if(NOT MSVC)
#     set(LINK_ARGS ${LINK_ARGS} hiredis pthread)
# endif()

add_executable(xpara_proxy_chain main.cpp)
# set_target_properties(xpara_proxy_chain
#     PROPERTIES OUTPUT_NAME "xtopchain"
# )
target_link_libraries(xpara_proxy_chain PRIVATE xpara_proxy)

# add_library(xtopchain SHARED so.cpp)
# set_target_properties(xtopchain PROPERTIES
#     VERSION   "${PROGRAM_VERSION}"
#     SOVERSION "${PROGRAM_VERSION}"
# )
# target_link_libraries(xtopchain PRIVATE xchaininit ${LINK_ARGS} xtopcl xxbase xdata xcrypto jsoncpp)

if (BUILD_METRICS)
    #add_dependencies(xtopchain xmetrics)
    # target_link_libraries(xtopchain PRIVATE xmetrics)
    #add_dependencies(xpara_proxy_chain xmetrics)
    target_link_libraries(xpara_proxy_chain PRIVATE xmetrics)
endif()
